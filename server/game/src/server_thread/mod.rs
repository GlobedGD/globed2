use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, Ordering},
        Arc,
    },
    time::{Duration, SystemTime},
};

use parking_lot::Mutex as SyncMutex;

use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, OsRng},
    ChaChaBox,
};
use log::{debug, warn};
use tokio::sync::{mpsc, Mutex};

use crate::{data::*, server::GameServer};

mod error;
mod handlers;

pub use error::{PacketHandlingError, Result};

// TODO adjust this to PlayerData size in the future plus some headroom
pub const SMALL_PACKET_LIMIT: usize = 128;
const CHANNEL_BUFFER_SIZE: usize = 8;

pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    SmallPacket([u8; SMALL_PACKET_LIMIT]),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    TerminationNotice(String),
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

    last_voice_packet: SyncMutex<SystemTime>,
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
            last_voice_packet: SyncMutex::new(SystemTime::now()),
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
                    Err(err) => warn!("[@{}]: {}", self.peer, err.to_string()),
                },
                Ok(None) | Err(_) => break, // sender closed | timeout
            };
        }
    }

    pub fn push_new_message(&self, data: ServerThreadMessage) -> anyhow::Result<()> {
        self.tx.try_send(data)?;
        Ok(())
    }

    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    /* private utilities */

    async fn send_packet<P: Packet>(&self, packet: &P) -> Result<()> {
        #[cfg(debug_assertions)]
        log::debug!("Sending {}", P::NAME);

        let serialized = self.serialize_packet(packet)?;
        self.send_buffer(serialized.as_bytes()).await
    }

    // fast packet sending with best-case zero heap allocation
    // packet must implement EncodableWithKnownSize to be fast sendable
    // it also must **NOT** be encrypted, for dynamically sized or encrypted packets, use send_packet.
    // on average 2-3x faster than send_packet, even worst case should be faster by a bit
    // TODO encryption
    async fn send_packet_fast<P: Packet + EncodableWithKnownSize>(&self, packet: &P) -> Result<()> {
        assert!(!P::ENCRYPTED, "Attempting to fast encode an encrypted packet ({})", P::NAME);

        let to_send: Result<Option<Vec<u8>>> = alloca::with_alloca(PacketHeader::SIZE + P::ENCODED_SIZE, move |data| {
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

    async fn send_buffer(&self, buffer: &[u8]) -> Result<()> {
        self.game_server
            .socket
            .send_to(buffer, self.peer)
            .await
            .map(|_size| ())
            .map_err(PacketHandlingError::SocketSendFailed)
    }

    // attempt to send a buffer immediately to the socket, but if it requires blocking then nuh uh
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

    fn serialize_packet<P: Packet>(&self, packet: &P) -> Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_packet_header::<P>();

        if !P::ENCRYPTED {
            buf.write_value(packet);
            return Ok(buf);
        }

        let cbox = self.crypto_box.lock();

        // should never happen
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

    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(data) => self.handle_packet(&data).await?,
            ServerThreadMessage::SmallPacket(data) => self.handle_packet(&data).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => self.send_packet(&*voice_packet).await?,
            ServerThreadMessage::TerminationNotice(message) => {
                self.terminate();
                self.send_packet(&ServerDisconnectPacket { message }).await?;
            }
        }

        Ok(())
    }

    /* packet handlers */

    async fn handle_packet(&self, message: &[u8]) -> Result<()> {
        #[cfg(debug_assertions)]
        if message.len() < PacketHeader::SIZE {
            return Err(PacketHandlingError::MalformedMessage);
        }

        let mut data = ByteReader::from_bytes(message);

        let header = data.read_packet_header()?;

        // minor optimization
        if header.packet_id == PlayerDataPacket::PACKET_ID {
            return self.handle_player_data(&mut data).await;
        }

        // also for optimization, reject the voice packet immediately if the player is blocked from vc
        if header.packet_id == VoicePacket::PACKET_ID {
            let accid = self.account_id.load(Ordering::Relaxed);
            if self.game_server.chat_blocked(accid) {
                debug!("blocking voice packet from {accid}");
                return Ok(());
            }
        }

        // reject cleartext credentials
        if header.packet_id == LoginPacket::PACKET_ID && !header.encrypted {
            return Err(PacketHandlingError::MalformedLoginAttempt);
        }

        let cleartext_vec: Vec<u8>;
        if header.encrypted {
            if message.len() < 24 + PacketHeader::SIZE {
                return Err(PacketHandlingError::MalformedCiphertext);
            }

            let cbox = self.crypto_box.lock();
            if cbox.is_none() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let cbox = cbox.as_ref().unwrap();

            let nonce = &message[PacketHeader::SIZE..PacketHeader::SIZE + 24];
            let ciphertext = &message[PacketHeader::SIZE + 24..];

            cleartext_vec = cbox
                .decrypt(nonce.into(), ciphertext)
                .map_err(|e| PacketHandlingError::DecryptionError(e.to_string()))?;
            data = ByteReader::from_bytes(&cleartext_vec);
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
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }
}
