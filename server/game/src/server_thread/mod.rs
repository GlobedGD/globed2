use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU64, Ordering},
        Arc,
    },
    time::Duration,
};

use parking_lot::Mutex as SyncMutex;

use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, AeadInPlace, OsRng},
    ChaChaBox,
};
use log::{debug, warn};
use tokio::sync::{mpsc, Mutex};

use crate::{data::*, server::GameServer};

mod error;
mod handlers;

pub use error::{PacketHandlingError, Result};

use self::handlers::MAX_VOICE_PACKET_SIZE;

// TODO adjust this to PlayerData size in the future plus some headroom
pub const SMALL_PACKET_LIMIT: usize = 164;
const CHANNEL_BUFFER_SIZE: usize = 8;
const NONCE_SIZE: usize = 24;
const MAC_SIZE: usize = 16;

pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    SmallPacket(([u8; SMALL_PACKET_LIMIT], u16)),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    BroadcastText(ChatMessageBroadcastPacket),
    TerminationNotice(FastString<MAX_NOTICE_SIZE>),
}

pub struct GameServerThread {
    game_server: &'static GameServer,

    rx: Mutex<mpsc::Receiver<ServerThreadMessage>>,
    tx: mpsc::Sender<ServerThreadMessage>,
    awaiting_termination: AtomicBool,
    pub authenticated: AtomicBool,
    crypto_box: SyncMutex<Option<ChaChaBox>>,

    peer: SocketAddrV4,
    pub account_id: AtomicI32,
    pub level_id: AtomicI32,
    pub account_data: SyncMutex<PlayerAccountData>,

    last_voice_packet: AtomicU64,
}

impl GameServerThread {
    /* public api for the main server */

    pub fn new(peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        let (tx, rx) = mpsc::channel::<ServerThreadMessage>(CHANNEL_BUFFER_SIZE);
        Self {
            tx,
            rx: Mutex::new(rx),
            peer,
            crypto_box: SyncMutex::new(None),
            account_id: AtomicI32::new(0),
            level_id: AtomicI32::new(0),
            authenticated: AtomicBool::new(false),
            game_server,
            awaiting_termination: AtomicBool::new(false),
            account_data: SyncMutex::new(PlayerAccountData::default()),
            last_voice_packet: AtomicU64::new(0),
        }
    }

    pub async fn run(&self) {
        let mut rx = self.rx.lock().await;

        loop {
            if self.awaiting_termination.load(Ordering::Relaxed) {
                break;
            }

            match tokio::time::timeout(Duration::from_secs(60), rx.recv()).await {
                Ok(Some(message)) => match self.handle_message(message).await {
                    Ok(()) => {}
                    Err(err) => warn!(
                        "[{} @ {}] err: {}",
                        self.account_id.load(Ordering::Relaxed),
                        self.peer,
                        err.to_string()
                    ),
                },
                Ok(None) | Err(_) => break, // sender closed | timeout
            };
        }
    }

    /// send a new message to this thread. keep in mind this blocks for a few microseconds.
    pub fn push_new_message(&self, data: ServerThreadMessage) -> anyhow::Result<()> {
        self.tx.try_send(data)?;
        Ok(())
    }

    /// terminate the thread as soon as possible and cleanup.
    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    /* private utilities */

    /// disconnect and send a message to the user
    async fn disconnect(&self, message: FastString<MAX_NOTICE_SIZE>) -> Result<()> {
        self.terminate();
        self.send_packet_fast(&ServerDisconnectPacket { message }).await
    }

    /// send a packet normally. with zero extra optimizations. like a sane person typically would.
    #[allow(dead_code)]
    async fn send_packet<P: Packet>(&self, packet: &P) -> Result<()> {
        #[cfg(debug_assertions)]
        log::debug!(
            "[{} @ {}] Sending packet {} (normal)",
            self.account_id.load(Ordering::Relaxed),
            self.peer,
            P::NAME
        );

        let serialized = self.serialize_packet(packet)?;
        self.send_buffer(serialized.as_bytes()).await
    }

    /// fast packet sending with best-case zero heap allocation.
    /// packet must not be encrypted and must implement `EncodableWithKnownSize` to be fast sendable,
    /// for dynamically sized packets use `send_packet` or `send_packet_fast_rough`,
    /// for encrypted packets use `send_packet_fast_encrypted`.
    /// on average, this is 2-3x faster than `send_packet`, even worst case should be faster by a bit.
    async fn send_packet_fast<P: Packet + EncodableWithKnownSize>(&self, packet: &P) -> Result<()> {
        self.send_packet_fast_rough(packet, P::ENCODED_SIZE).await
    }

    /// like `send_packet_fast` but without the size being known at compile time.
    /// you have to provide a rough estimate of the packet size, but if the packet doesn't fit, the function panics.
    async fn send_packet_fast_rough<P: Packet>(&self, packet: &P, packet_size: usize) -> Result<()> {
        assert!(
            !P::ENCRYPTED,
            "Attempting to fast encode an encrypted packet ({}), use 'send_packet_fast_encrypted' instead",
            P::NAME
        );

        #[cfg(debug_assertions)]
        log::debug!(
            "[{} @ {}] Sending packet {} (fast)",
            self.account_id.load(Ordering::Relaxed),
            self.peer,
            P::NAME
        );

        let to_send: Result<Option<Vec<u8>>> = alloca::with_alloca(PacketHeader::SIZE + packet_size, move |data| {
            // safety: 'data' will have garbage data but that is considered safe for all our intents and purposes
            // as `FastByteBuffer::as_bytes()` will only return what was already written.
            let data = unsafe {
                let ptr = data.as_mut_ptr().cast::<u8>();
                let len = std::mem::size_of_val(data);
                std::slice::from_raw_parts_mut(ptr, len)
            };

            let mut buf = FastByteBuffer::new(data);

            buf.write_packet_header::<P>();
            buf.write_value(packet);

            let send_data = buf.as_bytes();

            // we try a non-blocking send if we can, otherwise fallback to a Vec<u8> and an async send
            match self.send_buffer_immediate(send_data) {
                Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(send_data.to_vec())),
                Err(e) => Err(e),
                Ok(()) => Ok(None),
            }
        });

        if let Some(to_send) = to_send? {
            self.send_buffer(&to_send).await?;
        }

        Ok(())
    }

    /// fast encode & send an encrypted packet. hell yeah!
    async fn send_packet_fast_encrypted<P: Packet>(&self, packet: &P, packet_raw_size: usize) -> Result<()> {
        assert!(
            P::ENCRYPTED,
            "Attempting to call 'send_packet_fast_encrypted' on a non-encrypted packet ({})",
            P::NAME
        );

        #[cfg(debug_assertions)]
        log::debug!(
            "[{} @ {}] Sending packet {} (fast + encrypted)",
            self.account_id.load(Ordering::Relaxed),
            self.peer,
            P::NAME
        );

        let total_size = PacketHeader::SIZE + NONCE_SIZE + MAC_SIZE + packet_raw_size;
        let nonce_start = PacketHeader::SIZE;
        let mac_start = nonce_start + NONCE_SIZE;
        let raw_data_start = mac_start + MAC_SIZE;

        let to_send: Result<Option<Vec<u8>>> = alloca::with_alloca(total_size, move |data| {
            // safety: i don't even care anymore
            let data = unsafe {
                let ptr = data.as_mut_ptr().cast::<u8>();
                let len = std::mem::size_of_val(data);
                std::slice::from_raw_parts_mut(ptr, len)
            };

            // write the header
            let mut buf = FastByteBuffer::new(data);
            buf.write_packet_header::<P>();

            // first encode the packet
            let mut buf = FastByteBuffer::new(&mut data[raw_data_start..raw_data_start + packet_raw_size]);
            buf.write_value(packet);

            // if the written size isn't equal to `packet_raw_size`, we use buffer length instead
            let raw_data_end = raw_data_start + buf.len();

            // encrypt in place
            let cbox = self.crypto_box.lock();
            if cbox.is_none() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let cbox = cbox.as_ref().unwrap();
            let nonce = ChaChaBox::generate_nonce(&mut OsRng);
            let tag = cbox
                .encrypt_in_place_detached(&nonce, b"", &mut data[raw_data_start..raw_data_end])
                .map_err(|e| PacketHandlingError::EncryptionError(e.to_string()))?;

            // prepend the nonce
            data[nonce_start..mac_start].copy_from_slice(&nonce);

            // prepend the mac tag
            data[mac_start..raw_data_start].copy_from_slice(&tag);

            // we try a non-blocking send if we can, otherwise fallback to a Vec<u8> and an async send
            let send_data = &data[..raw_data_end];
            match self.send_buffer_immediate(send_data) {
                Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(send_data.to_vec())),
                Err(e) => Err(e),
                Ok(()) => Ok(None),
            }
        });

        if let Some(to_send) = to_send? {
            self.send_buffer(&to_send).await?;
        }

        Ok(())
    }

    /// sends a buffer to our peer via the socket
    async fn send_buffer(&self, buffer: &[u8]) -> Result<()> {
        self.game_server
            .socket
            .send_to(buffer, self.peer)
            .await
            .map(|_size| ())
            .map_err(PacketHandlingError::SocketSendFailed)
    }

    /// attempt to send a buffer immediately to the socket, but if it requires blocking then returns an error
    fn send_buffer_immediate(&self, buffer: &[u8]) -> Result<()> {
        self.game_server
            .socket
            .try_send_to(buffer, std::net::SocketAddr::V4(self.peer))
            .map(|_| ())
            .map_err(|e| {
                if e.kind() == std::io::ErrorKind::WouldBlock {
                    PacketHandlingError::SocketWouldBlock
                } else {
                    PacketHandlingError::SocketSendFailed(e)
                }
            })
    }

    /// serialize (and potentially encrypt) a packet, returning a `ByteBuffer` with the output data
    #[allow(dead_code)]
    fn serialize_packet<P: Packet>(&self, packet: &P) -> Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_packet_header::<P>();

        if !P::ENCRYPTED {
            buf.write_value(packet);
            return Ok(buf);
        }

        let cbox = self.crypto_box.lock();

        // should be impossible to happen
        #[cfg(debug_assertions)]
        if !cbox.is_some() {
            return Err(PacketHandlingError::WrongCryptoBoxState);
        }

        // encode the packet into a temp buf, encrypt the data and write to the initial buf

        let mut cltxtbuf = ByteBuffer::new();
        cltxtbuf.write_value(packet);

        let cbox = cbox.as_ref().unwrap();
        let nonce = ChaChaBox::generate_nonce(&mut OsRng);

        let encrypted = cbox
            .encrypt(&nonce, cltxtbuf.as_bytes())
            .map_err(|e| PacketHandlingError::EncryptionError(e.to_string()))?;

        #[cfg(not(rust_analyzer))] // i am so sorry
        buf.write_bytes(&nonce);
        buf.write_bytes(&encrypted);

        Ok(buf)
    }

    /// handle a message sent from the `GameServer`
    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(mut data) => self.handle_packet(&mut data).await?,
            ServerThreadMessage::SmallPacket((mut data, len)) => self.handle_packet(&mut data[..len as usize]).await?,
            ServerThreadMessage::BroadcastText(text_packet) => self.send_packet_fast(&text_packet).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => {
                let calc_size = voice_packet.data.data.len() + size_of_types!(u32);
                self.send_packet_fast_encrypted(&*voice_packet, calc_size).await?;
            }
            ServerThreadMessage::TerminationNotice(message) => self.disconnect(message).await?,
        }

        Ok(())
    }

    /// handle an incoming packet
    async fn handle_packet(&self, message: &mut [u8]) -> Result<()> {
        if message.len() < PacketHeader::SIZE {
            return Err(PacketHandlingError::MalformedMessage);
        }

        let mut data = ByteReader::from_bytes(message);
        let header = data.read_packet_header()?;

        // minor optimization
        if header.packet_id == PlayerDataPacket::PACKET_ID {
            return self.handle_player_data(&mut data).await;
        }

        // also for optimization, reject the voice packet immediately on certain conditions
        if header.packet_id == VoicePacket::PACKET_ID {
            let accid = self.account_id.load(Ordering::Relaxed);
            if self.game_server.chat_blocked(accid) {
                debug!("blocking voice packet from {accid}");
                return Ok(());
            }

            if message.len() > MAX_VOICE_PACKET_SIZE {
                debug!(
                    "blocking voice packet from {accid} because it's too big ({} bytes)",
                    message.len()
                );
                return Ok(());
            }
        }

        // reject cleartext credentials
        if header.packet_id == LoginPacket::PACKET_ID && !header.encrypted {
            return Err(PacketHandlingError::MalformedLoginAttempt);
        }

        if header.encrypted {
            if message.len() < PacketHeader::SIZE + NONCE_SIZE + MAC_SIZE {
                return Err(PacketHandlingError::MalformedCiphertext);
            }

            let cbox = self.crypto_box.lock();
            if cbox.is_none() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let cbox = cbox.as_ref().unwrap();

            let nonce_start = PacketHeader::SIZE;
            let mac_start = nonce_start + NONCE_SIZE;
            let ciphertext_start = mac_start + MAC_SIZE;

            let mut nonce = [0u8; NONCE_SIZE];
            nonce[..NONCE_SIZE].clone_from_slice(&message[nonce_start..mac_start]);
            let nonce = nonce.into();

            let mut mac = [0u8; MAC_SIZE];
            mac[..MAC_SIZE].clone_from_slice(&message[mac_start..ciphertext_start]);
            let mac = mac.into();

            cbox.decrypt_in_place_detached(&nonce, b"", &mut message[ciphertext_start..], &mac)
                .map_err(|e| PacketHandlingError::DecryptionError(e.to_string()))?;

            data = ByteReader::from_bytes(&message[ciphertext_start..]);
        }

        match header.packet_id {
            /* connection related */
            PingPacket::PACKET_ID => self.handle_ping(&mut data).await,
            CryptoHandshakeStartPacket::PACKET_ID => self.handle_crypto_handshake(&mut data).await,
            KeepalivePacket::PACKET_ID => self.handle_keepalive(&mut data).await,
            LoginPacket::PACKET_ID => self.handle_login(&mut data).await,
            DisconnectPacket::PACKET_ID => self.handle_disconnect(&mut data),

            /* game related */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,
            RequestPlayerListPacket::PACKET_ID => self.handle_request_player_list(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            ChatMessagePacket::PACKET_ID => self.handle_chat_message(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }
}
