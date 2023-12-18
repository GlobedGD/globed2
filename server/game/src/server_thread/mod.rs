use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU32, AtomicU64, Ordering},
        Arc, OnceLock,
    },
    time::{Duration, SystemTime, UNIX_EPOCH},
};

use parking_lot::Mutex as SyncMutex;

use esp::ByteReader;
use globed_shared::{
    anyhow,
    crypto_box::{
        aead::{AeadCore, AeadInPlace, OsRng},
        ChaChaBox,
    },
    logger::*,
};

use crate::{data::*, make_uninit, server::GameServer, server_thread::handlers::*, util::TokioChannel};

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
    crypto_box: OnceLock<ChaChaBox>,

    peer: SocketAddrV4,
    pub account_id: AtomicI32,
    pub level_id: AtomicI32,
    pub room_id: AtomicU32,
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
            room_id: AtomicU32::new(0),
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

    pub fn authenticated(&self) -> bool {
        self.account_id.load(Ordering::Relaxed) != 0
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

    // the error printing is different in release and debug. some errors have higher severity than others.
    fn print_error(&self, error: &PacketHandlingError) {
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

    #[cfg(debug_assertions)]
    fn print_packet<P: PacketMetadata>(&self, sending: bool, ptype: Option<&str>) {
        trace!(
            "[{}] {} packet {}{}",
            {
                let name = self.account_data.lock().name.clone();
                if name.is_empty() {
                    self.peer.to_string()
                } else {
                    format!("{name} @ {}", self.peer)
                }
            },
            if sending { "Sending" } else { "Handling" },
            P::NAME,
            ptype.map_or(String::new(), |pt| format!(" ({pt})"))
        );
    }

    #[inline]
    #[cfg(not(debug_assertions))]
    fn print_packet<P: PacketMetadata>(&self, _sending: bool, _ptype: Option<&str>) {}

    /// call `self.terminate()` and send a message to the user with the reason
    async fn disconnect(&self, message: &str) -> Result<()> {
        self.terminate();
        self.send_packet_dynamic(&ServerDisconnectPacket { message }).await
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

    fn is_chat_packet_allowed(&self, voice: bool, len: usize) -> bool {
        let accid = self.account_id.load(Ordering::Relaxed);
        if accid == 0 {
            // unauthorized
            return false;
        }

        if self.game_server.chat_blocked(accid) {
            // blocked from chat
            return false;
        }

        if !voice {
            // chat packet - is permitted at this point
            return true;
        }

        if len > MAX_VOICE_PACKET_SIZE {
            // voice packet is too big
            return false;
        }

        // check the throughput
        let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64;
        let last_voice_packet = self.last_voice_packet.swap(now, Ordering::Relaxed);
        let mut passed_time = now - last_voice_packet;

        if passed_time == 0 {
            passed_time = 1;
        }

        let throughput = len / passed_time as usize; // in kb per second

        if throughput > MAX_VOICE_THROUGHPUT {
            #[cfg(debug_assertions)]
            warn!("rejecting a voice packet, throughput above the limit: {}kb/s", throughput);
            return false;
        }

        true
    }

    /// handle a message sent from the `GameServer`
    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(mut data) => self.handle_packet(&mut data).await?,
            ServerThreadMessage::SmallPacket((mut data, len)) => self.handle_packet(&mut data[..len as usize]).await?,
            ServerThreadMessage::BroadcastText(text_packet) => self.send_packet_static(&text_packet).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => self.send_packet_dynamic(&*voice_packet).await?,
            ServerThreadMessage::TerminationNotice(message) => self.disconnect(message.try_to_str()).await?,
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

        // by far the most common packet, so we try it early
        if header.packet_id == PlayerDataPacket::PACKET_ID {
            return self.handle_player_data(&mut data).await;
        }

        // also for optimization, reject the voice/text packet immediately on certain conditions
        if (header.packet_id == VoicePacket::PACKET_ID || header.packet_id == ChatMessagePacket::PACKET_ID)
            && !self.is_chat_packet_allowed(header.packet_id == VoicePacket::PACKET_ID, message.len())
        {
            #[cfg(debug_assertions)]
            log::warn!("blocking text/voice packet from {}", self.account_id.load(Ordering::Relaxed));
            return Ok(());
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
            nonce.clone_from_slice(&message[nonce_start..mac_start]);
            let nonce = nonce.into();

            let mut mac = [0u8; MAC_SIZE];
            mac.clone_from_slice(&message[mac_start..ciphertext_start]);
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

            /* general */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestGlobalPlayerListPacket::PACKET_ID => self.handle_request_global_list(&mut data).await,
            CreateRoomPacket::PACKET_ID => self.handle_create_room(&mut data).await,
            JoinRoomPacket::PACKET_ID => self.handle_join_room(&mut data).await,
            LeaveRoomPacket::PACKET_ID => self.handle_leave_room(&mut data).await,
            RequestRoomPlayerListPacket::PACKET_ID => self.handle_request_room_list(&mut data).await,

            /* game related */
            RequestPlayerProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            ChatMessagePacket::PACKET_ID => self.handle_chat_message(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }

    // packet encoding and sending functions

    /// fast packet sending with best-case zero heap allocation. requires the packet to implement `StaticSize`.
    /// if the packet size isn't known at compile time, derive/implement `DynamicSize` and use `send_packet_dynamic` instead.
    #[inline(never)]
    async fn send_packet_static<P: Packet + Encodable + StaticSize>(&self, packet: &P) -> Result<()> {
        // in theory, the size is known at compile time, so we could use a stack array here, instead of using alloca.
        // however in practice, the performance difference is negligible, so we avoid code unnecessary code repetition.
        self.send_packet_alloca(packet, P::ENCODED_SIZE).await
    }

    /// version of `send_packet_static` that does not require the size to be known at compile time.
    /// you are still required to derive/implement `DynamicSize` so the size can be computed at runtime.
    #[inline(never)]
    async fn send_packet_dynamic<P: Packet + Encodable + DynamicSize>(&self, packet: &P) -> Result<()> {
        self.send_packet_alloca(packet, packet.encoded_size()).await
    }

    /// use alloca to encode the packet on the stack, and try a non-blocking send, on failure clones to a Vec and a blocking send.
    /// be very careful if using this directly, miscalculating the size will cause a runtime panic.
    #[inline]
    async fn send_packet_alloca<P: Packet + Encodable>(&self, packet: &P, packet_size: usize) -> Result<()> {
        self.send_packet_alloca_with::<P, _>(packet_size, |buf| {
            buf.write_value(packet);
        })
        .await
    }

    /// low level version of other `send_packet_xxx` methods. there is no bound for `Encodable`, `StaticSize` or `DynamicSize`,
    /// but you have to provide a closure that handles packet encoding, and you must specify the appropriate packet size.
    /// you do **not** have to encode the packet header or include its size in the `packet_size` argument, that will be done for you automatically.
    #[inline]
    async fn send_packet_alloca_with<P: Packet, F>(&self, packet_size: usize, encode_fn: F) -> Result<()>
    where
        F: FnOnce(&mut FastByteBuffer),
    {
        if cfg!(debug_assertions)
            && P::PACKET_ID != KeepaliveResponsePacket::PACKET_ID
            && P::PACKET_ID != LevelDataPacket::PACKET_ID
        {
            self.print_packet::<P>(true, Some(if P::ENCRYPTED { "fast + encrypted" } else { "fast" }));
        }

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
                encode_fn(&mut buf);

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
                encode_fn(&mut buf);
            });
        }

        Ok(())
    }
}
