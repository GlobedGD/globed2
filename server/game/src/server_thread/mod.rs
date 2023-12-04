use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU64, Ordering},
        Arc, OnceLock,
    },
    time::Duration,
};

use parking_lot::Mutex as SyncMutex;

use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, AeadInPlace, OsRng},
    ChaChaBox,
};
use globed_shared::logger::*;

use crate::{data::*, server::GameServer, server_thread::handlers::*, util::TokioChannel};

mod error;
mod handlers;

pub use error::{PacketHandlingError, Result};

use self::handlers::MAX_VOICE_PACKET_SIZE;

// TODO adjust this to PlayerData size in the future plus some headroom
pub const SMALL_PACKET_LIMIT: usize = 164;
const CHANNEL_BUFFER_SIZE: usize = 8;

// do not touch those, encryption related
const NONCE_SIZE: usize = 24;
const MAC_SIZE: usize = 16;

#[derive(Clone)]
pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    SmallPacket(([u8; SMALL_PACKET_LIMIT], u16)),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    BroadcastText(ChatMessageBroadcastPacket),
    TerminationNotice(FastString<MAX_NOTICE_SIZE>),
}

pub struct GameServerThread {
    game_server: &'static GameServer,

    channel: TokioChannel<ServerThreadMessage>,
    awaiting_termination: AtomicBool,
    pub authenticated: AtomicBool,
    crypto_box: OnceLock<ChaChaBox>,

    peer: SocketAddrV4,
    pub account_id: AtomicI32,
    pub level_id: AtomicI32,
    pub account_data: SyncMutex<PlayerAccountData>,

    last_voice_packet: AtomicU64,
}

impl GameServerThread {
    /* public api for the main server */

    pub fn new(peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        Self {
            channel: TokioChannel::new(CHANNEL_BUFFER_SIZE),
            peer,
            crypto_box: OnceLock::new(),
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
        loop {
            if self.awaiting_termination.load(Ordering::Relaxed) {
                break;
            }

            // safety: we are the only receiver for this channel.
            match tokio::time::timeout(Duration::from_secs(60), unsafe { self.channel.recv() }).await {
                Ok(Ok(message)) => match self.handle_message(message).await {
                    Ok(()) => {}
                    Err(err) => self.print_error(&err),
                },
                Ok(Err(_)) | Err(_) => break, // sender closed | timeout
            };
        }
    }

    /// the error printing is different in release and debug. some errors have higher severity than others.
    pub fn print_error(&self, error: &PacketHandlingError) {
        if cfg!(debug_assertions) {
            warn!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.peer, error);
        } else {
            match error {
                // these are for the client being silly
                PacketHandlingError::Other(_)
                | PacketHandlingError::WrongCryptoBoxState
                | PacketHandlingError::EncryptionError
                | PacketHandlingError::DecryptionError
                | PacketHandlingError::IOError(_) => {
                    warn!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.peer, error);
                }
                // these are either our fault or a fatal error somewhere
                PacketHandlingError::SocketSendFailed(_)
                | PacketHandlingError::ColorParseFailed(_)
                | PacketHandlingError::UnexpectedCentralResponse
                | PacketHandlingError::SystemTimeError(_)
                | PacketHandlingError::WebRequestError(_)
                | PacketHandlingError::DangerousAllocation(_) => {
                    error!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.peer, error);
                }
                // these can likely never happen unless network corruption or someone is pentesting, so ignore in release
                PacketHandlingError::MalformedMessage
                | PacketHandlingError::MalformedCiphertext
                | PacketHandlingError::MalformedLoginAttempt
                | PacketHandlingError::MalformedPacketStructure(_)
                | PacketHandlingError::NoHandler(_)
                | PacketHandlingError::SocketWouldBlock
                | PacketHandlingError::Ratelimited
                | PacketHandlingError::UnexpectedPlayerData => {}
            }
        }
    }

    /// send a new message to this thread.
    pub fn push_new_message(&self, data: ServerThreadMessage) -> anyhow::Result<()> {
        self.channel.try_send(data)?;
        Ok(())
    }

    /// schedule the thread to terminate as soon as possible.
    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    /* private utilities */

    /// call `self.terminate()` and send a message to the user
    async fn disconnect(&self, message: FastString<MAX_NOTICE_SIZE>) -> Result<()> {
        self.terminate();
        self.send_packet_fast(&ServerDisconnectPacket { message }).await
    }

    /// send a packet normally. with zero extra optimizations. like a sane person typically would.
    #[allow(dead_code)]
    async fn send_packet<P: Packet + Encodable>(&self, packet: &P) -> Result<()> {
        #[cfg(debug_assertions)]
        debug!(
            "[{} @ {}] Sending packet {} (normal)",
            self.account_id.load(Ordering::Relaxed),
            self.peer,
            P::NAME
        );

        let serialized = self.serialize_packet(packet)?;
        self.send_buffer(serialized.as_bytes()).await
    }

    /// fast packet sending with best-case zero heap allocation.
    /// if the packet size isn't known at compile time, calculate it and use `send_packet_fast_rough`
    async fn send_packet_fast<P: Packet + Encodable + KnownSize>(&self, packet: &P) -> Result<()> {
        // in theory, the size is known at compile time, however for some reason,
        // alloca manages to be significantly faster than a `[MaybeUninit<u8>; N]`.
        // i have no idea why or how, but yeah.
        self.send_packet_fast_rough(packet, P::ENCODED_SIZE).await
    }

    /// like `send_packet_fast` but without the size being known at compile time.
    /// you have to provide a rough estimate of the packet size, if the packet doesn't fit, the function panics.
    async fn send_packet_fast_rough<P: Packet + Encodable>(&self, packet: &P, packet_size: usize) -> Result<()> {
        #[cfg(debug_assertions)]
        debug!(
            "[{} @ {}] Sending packet {} ({})",
            self.account_id.load(Ordering::Relaxed),
            self.peer,
            P::NAME,
            if P::ENCRYPTED { "fast + encrypted" } else { "fast" }
        );

        if P::ENCRYPTED {
            // gs_inline_encode! doesn't work here because the borrow checker is silly :(
            let total_size = PacketHeader::SIZE + NONCE_SIZE + MAC_SIZE + packet_size;
            let nonce_start = PacketHeader::SIZE;
            let mac_start = nonce_start + NONCE_SIZE;
            let raw_data_start = mac_start + MAC_SIZE;

            gs_alloca_check_size!(total_size);

            let to_send: Result<Option<Vec<u8>>> = gs_with_alloca!(total_size, data, {
                // write the header
                let mut buf = FastByteBuffer::new(data);
                buf.write_packet_header::<P>();

                // first encode the packet
                let mut buf = FastByteBuffer::new(&mut data[raw_data_start..raw_data_start + packet_size]);
                buf.write_value(packet);

                // if the written size isn't equal to `packet_size`, we use buffer length instead
                let raw_data_end = raw_data_start + buf.len();

                // this unwrap is safe, as an encrypted packet can only be sent downstream after the handshake is established.
                let cbox = self.crypto_box.get().unwrap();

                // encrypt in place
                let nonce = ChaChaBox::generate_nonce(&mut OsRng);
                let tag = cbox
                    .encrypt_in_place_detached(&nonce, b"", &mut data[raw_data_start..raw_data_end])
                    .map_err(|_| PacketHandlingError::EncryptionError)?;

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
        } else {
            gs_inline_encode!(self, PacketHeader::SIZE + packet_size, buf, {
                buf.write_packet_header::<P>();
                buf.write_value(packet);
            });
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
    fn serialize_packet<P: Packet + Encodable>(&self, packet: &P) -> Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_packet_header::<P>();

        if !P::ENCRYPTED {
            buf.write_value(packet);
            return Ok(buf);
        }

        // this unwrap is safe, as an encrypted packet can only be sent downstream after the handshake is established.
        let cbox = self.crypto_box.get().unwrap();

        // encode the packet into a temp buf, encrypt the data and write to the initial buf

        let mut cltxtbuf = ByteBuffer::new();
        cltxtbuf.write_value(packet);

        let nonce = ChaChaBox::generate_nonce(&mut OsRng);

        let encrypted = cbox
            .encrypt(&nonce, cltxtbuf.as_bytes())
            .map_err(|_| PacketHandlingError::EncryptionError)?;

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
                self.send_packet_fast_rough(&*voice_packet, calc_size).await?;
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
        if header.packet_id == VoicePacket::PACKET_ID || header.packet_id == ChatMessagePacket::PACKET_ID {
            let accid = self.account_id.load(Ordering::Relaxed);
            if self.game_server.chat_blocked(accid) {
                debug!("blocking voice packet from {accid}");
                return Ok(());
            }

            if header.packet_id == VoicePacket::PACKET_ID && message.len() > MAX_VOICE_PACKET_SIZE {
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

        // decrypt the packet in-place if encrypted
        if header.encrypted {
            if message.len() < PacketHeader::SIZE + NONCE_SIZE + MAC_SIZE {
                return Err(PacketHandlingError::MalformedCiphertext);
            }

            let cbox = self.crypto_box.get();
            if cbox.is_none() {
                self.terminate();
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let cbox = cbox.unwrap();

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
                .map_err(|_| PacketHandlingError::DecryptionError)?;

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
            SyncPlayerMetadataPacket::PACKET_ID => self.handle_sync_player_metadata(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            ChatMessagePacket::PACKET_ID => self.handle_chat_message(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }
}