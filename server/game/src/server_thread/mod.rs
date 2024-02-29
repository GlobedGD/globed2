use std::{
    collections::VecDeque,
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU16, AtomicU32, Ordering},
        Arc, OnceLock,
    },
    time::Duration,
};

use esp::ByteReader;
use globed_shared::{
    crypto_box::{
        aead::{AeadCore, AeadInPlace, OsRng},
        ChaChaBox,
    },
    logger::*,
    SyncMutex, UserEntry,
};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::TcpStream,
    sync::{Mutex, Notify},
};

use crate::{
    data::*,
    make_uninit,
    server::GameServer,
    server_thread::handlers::*,
    util::{LockfreeMutCell, SimpleRateLimiter},
};

mod error;
pub mod handlers;

pub use error::{PacketHandlingError, Result};

pub const INLINE_BUFFER_SIZE: usize = 164;

const MAX_PACKET_SIZE: usize = 65536;

// do not touch those, encryption related
const NONCE_SIZE: usize = 24;
const MAC_SIZE: usize = 16;

#[derive(Clone)]
pub enum ServerThreadMessage {
    SmallPacket(([u8; INLINE_BUFFER_SIZE], usize)),
    Packet(Vec<u8>),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    BroadcastText(ChatMessageBroadcastPacket),
    BroadcastNotice(ServerNoticePacket),
    TerminationNotice(FastString<MAX_NOTICE_SIZE>),
}

pub struct GameServerThread {
    game_server: &'static GameServer,
    pub socket: LockfreeMutCell<TcpStream>,

    pub claimed: AtomicBool,
    pub claim_secret_key: AtomicU32,

    awaiting_termination: AtomicBool,
    crypto_box: OnceLock<ChaChaBox>,

    pub tcp_peer: SocketAddrV4,
    pub udp_peer: SyncMutex<SocketAddrV4>,
    pub is_authorized_admin: AtomicBool,
    pub account_id: AtomicI32,
    pub level_id: AtomicLevelId,
    pub room_id: AtomicU32,
    pub account_data: SyncMutex<PlayerAccountData>,
    pub user_entry: SyncMutex<UserEntry>,

    pub cleanup_notify: Notify,
    pub cleanup_mutex: Mutex<()>,
    fragmentation_limit: AtomicU16,

    message_queue: Mutex<VecDeque<ServerThreadMessage>>,
    message_notify: Notify,
    rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
}

impl GameServerThread {
    /* public api for the main server */

    pub fn new(socket: TcpStream, peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        let rl_request_limit = (game_server.bridge.central_conf.lock().tps + 6) as usize;
        let rate_limiter = SimpleRateLimiter::new(rl_request_limit, Duration::from_millis(925));

        Self {
            game_server,
            socket: LockfreeMutCell::new(socket),
            claimed: AtomicBool::new(false),
            claim_secret_key: AtomicU32::new(0),
            tcp_peer: peer,
            udp_peer: SyncMutex::new(peer),
            crypto_box: OnceLock::new(),
            is_authorized_admin: AtomicBool::new(false),
            account_id: AtomicI32::new(0),
            level_id: AtomicLevelId::new(0),
            room_id: AtomicU32::new(0),
            awaiting_termination: AtomicBool::new(false),
            account_data: SyncMutex::new(PlayerAccountData::default()),
            user_entry: SyncMutex::new(UserEntry::default()),
            cleanup_notify: Notify::new(),
            cleanup_mutex: Mutex::new(()),
            fragmentation_limit: AtomicU16::new(0),
            message_queue: Mutex::new(VecDeque::new()),
            message_notify: Notify::new(),
            rate_limiter: LockfreeMutCell::new(rate_limiter),
        }
    }

    async fn poll_for_messages(&self) -> Option<ServerThreadMessage> {
        {
            let mut mq = self.message_queue.lock().await;
            if !mq.is_empty() {
                return mq.pop_front();
            }
        }

        self.message_notify.notified().await;
        self.message_queue.lock().await.pop_front()
    }

    async fn poll_for_tcp_data(&self) -> Result<usize> {
        let tcp_socket = unsafe { self.socket.get_mut() };

        let mut length_buf = [0u8; 4];
        tcp_socket.read_exact(&mut length_buf).await?;

        Ok(u32::from_be_bytes(length_buf) as usize)
    }

    pub async fn run(&self) {
        loop {
            if self.awaiting_termination.load(Ordering::Relaxed) {
                break;
            }

            tokio::select! {
                message = self.poll_for_messages() => {
                    if let Some(message) = message {
                        match self.handle_message(message).await {
                            Ok(()) => {}
                            Err(e) => self.print_error(&e),
                        }
                    }
                }

                timeout_res = tokio::time::timeout(Duration::from_secs(90), self.poll_for_tcp_data()) => match timeout_res {
                    Ok(Ok(message_len)) => {
                        match self.recv_and_handle(message_len).await {
                            Ok(()) => {}
                            Err(e) => self.print_error(&e),
                        }
                    }
                    Ok(Err(err)) => {
                        // early eof means the client has disconnected (closed their part of the socketS)
                        if let PacketHandlingError::IOError(ref ioerror) = err {
                            if ioerror.kind() != std::io::ErrorKind::UnexpectedEof {
                                self.print_error(&err);
                            }
                        }
                        break;
                    }
                    Err(_) => {
                        debug!("tcp connection not responding for 90 seconds, terminating");
                        break;
                    }
                }
            };
        }
    }

    pub fn authenticated(&self) -> bool {
        self.account_id.load(Ordering::Relaxed) != 0
    }

    /// schedule the thread to terminate as soon as possible.
    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    pub async fn push_new_message(&self, message: ServerThreadMessage) {
        self.message_queue.lock().await.push_back(message);
        self.message_notify.notify_one();
    }

    /* private utilities */

    // the error printing is different in release and debug. some errors have higher severity than others.
    fn print_error(&self, error: &PacketHandlingError) {
        if cfg!(debug_assertions) {
            warn!(
                "[{} @ {}] err: {}",
                self.account_id.load(Ordering::Relaxed),
                self.tcp_peer,
                error
            );
        } else {
            match error {
                // these are for the client being silly
                PacketHandlingError::Other(_)
                | PacketHandlingError::WrongCryptoBoxState
                | PacketHandlingError::EncryptionError
                | PacketHandlingError::DecryptionError
                | PacketHandlingError::NoHandler(_)
                | PacketHandlingError::IOError(_)
                | PacketHandlingError::DebugOnlyPacket
                | PacketHandlingError::PacketTooLong(_)
                | PacketHandlingError::SocketSendFailed(_) => {
                    warn!(
                        "[{} @ {}] err: {}",
                        self.account_id.load(Ordering::Relaxed),
                        self.tcp_peer,
                        error
                    );
                }
                // these are either our fault or a fatal error somewhere
                PacketHandlingError::ColorParseFailed(_)
                | PacketHandlingError::UnexpectedCentralResponse
                | PacketHandlingError::SystemTimeError(_)
                | PacketHandlingError::WebRequestError(_)
                | PacketHandlingError::DangerousAllocation(_) => {
                    error!(
                        "[{} @ {}] err: {}",
                        self.account_id.load(Ordering::Relaxed),
                        self.tcp_peer,
                        error
                    );
                }
                // these can likely never happen unless network corruption or someone is pentesting, so ignore in release
                PacketHandlingError::MalformedMessage
                | PacketHandlingError::MalformedCiphertext
                | PacketHandlingError::MalformedLoginAttempt
                | PacketHandlingError::MalformedPacketStructure(_)
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
                    self.tcp_peer.to_string()
                } else {
                    format!("{name} @ {}", self.tcp_peer)
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

    /// sends a buffer to our peer via the tcp socket
    async fn send_buffer_tcp(&self, buffer: &[u8]) -> Result<()> {
        // safety: only we can send data to our client.
        let socket = unsafe { self.socket.get_mut() };
        let result = tokio::time::timeout(Duration::from_secs(5), socket.write_all(buffer)).await;

        match result {
            Ok(Ok(())) => Ok(()),
            Ok(Err(err)) => Err(PacketHandlingError::SocketSendFailed(err)),
            Err(_) => {
                // timed out
                Err(PacketHandlingError::SocketSendFailed(std::io::Error::new(
                    std::io::ErrorKind::TimedOut,
                    "TCP send timed out (waited for 5 seconds)",
                )))
            }
        }
    }

    /// non async version of `send_buffer_tcp`
    fn send_buffer_tcp_immediate(&self, buffer: &[u8]) -> Result<usize> {
        // safety: only we can send data to our client.
        let socket = unsafe { self.socket.get_mut() };
        let result = socket.try_write(buffer);

        match result {
            Ok(x) => Ok(x),
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => Err(PacketHandlingError::SocketWouldBlock),
            Err(e) => Err(e.into()),
        }
    }

    /// sends a buffer to our peer via the udp socket
    async fn send_buffer_udp(&self, buffer: &[u8]) -> Result<()> {
        let udp_addr = { *self.udp_peer.lock() };

        self.game_server
            .udp_socket
            .send_to(buffer, udp_addr)
            .await
            .map(|_size| ())
            .map_err(PacketHandlingError::SocketSendFailed)
    }

    /// non async version of `send_buffer_udp`
    fn send_buffer_udp_immediate(&self, buffer: &[u8]) -> Result<usize> {
        self.game_server
            .udp_socket
            .try_send_to(buffer, std::net::SocketAddr::V4(*self.udp_peer.lock()))
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

        if self.user_entry.lock().is_muted {
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

        true
    }

    async fn recv_and_handle(&self, message_size: usize) -> Result<()> {
        // safety: only we can receive data from our client.
        let socket = unsafe { self.socket.get_mut() };

        if message_size > MAX_PACKET_SIZE {
            return Err(PacketHandlingError::PacketTooLong(message_size));
        }

        let data: &mut [u8];

        let mut inline_buf = [0u8; INLINE_BUFFER_SIZE];
        let mut heap_buf = Vec::<u8>::new();

        let use_inline = message_size <= INLINE_BUFFER_SIZE;

        if use_inline {
            data = &mut inline_buf[..message_size];
            socket.read_exact(data).await?;
        } else {
            let read_bytes = socket.take(message_size as u64).read_to_end(&mut heap_buf).await?;
            data = &mut heap_buf[..read_bytes];
        }

        self.handle_packet(data).await?;

        Ok(())
    }

    /// handle a message sent from the `GameServer`
    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(mut packet) => self.handle_packet(&mut packet).await?,
            ServerThreadMessage::SmallPacket((mut packet, len)) => self.handle_packet(&mut packet[..len]).await?,
            ServerThreadMessage::BroadcastText(text_packet) => self.send_packet_static(&text_packet).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => self.send_packet_dynamic(&*voice_packet).await?,
            ServerThreadMessage::BroadcastNotice(packet) => {
                self.send_packet_static(&packet).await?;
                info!("{} is receiving a notice: {}", self.account_data.lock().name, packet.message);
            }
            ServerThreadMessage::TerminationNotice(message) => self.disconnect(message.try_to_str()).await?,
        }

        Ok(())
    }

    /// handle an incoming packet
    async fn handle_packet(&self, message: &mut [u8]) -> Result<()> {
        if message.len() < PacketHeader::SIZE {
            return Err(PacketHandlingError::MalformedMessage);
        }

        // if we are ratelimited, just discard the packet.
        // safety: only we can use this ratelimiter.
        if !unsafe { self.rate_limiter.get_mut() }.try_tick() {
            return Err(PacketHandlingError::Ratelimited);
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
            ConnectionTestPacket::PACKET_ID => self.handle_connection_test(&mut data).await,
            KeepaliveTCPPacket::PACKET_ID => self.handle_keepalive_tcp(&mut data).await,

            /* general */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestGlobalPlayerListPacket::PACKET_ID => self.handle_request_global_list(&mut data).await,
            CreateRoomPacket::PACKET_ID => self.handle_create_room(&mut data).await,
            JoinRoomPacket::PACKET_ID => self.handle_join_room(&mut data).await,
            LeaveRoomPacket::PACKET_ID => self.handle_leave_room(&mut data).await,
            RequestRoomPlayerListPacket::PACKET_ID => self.handle_request_room_list(&mut data).await,
            RequestLevelListPacket::PACKET_ID => self.handle_request_level_list(&mut data).await,
            RequestPlayerCountPacket::PACKET_ID => self.handle_request_player_count(&mut data).await,

            /* game related */
            RequestPlayerProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,
            PlayerMetadataPacket::PACKET_ID => self.handle_player_metadata(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            ChatMessagePacket::PACKET_ID => self.handle_chat_message(&mut data).await,

            /* admin related */
            AdminAuthPacket::PACKET_ID => self.handle_admin_auth(&mut data).await,
            AdminSendNoticePacket::PACKET_ID => self.handle_admin_send_notice(&mut data).await,
            AdminDisconnectPacket::PACKET_ID => self.handle_admin_disconnect(&mut data).await,
            AdminGetUserStatePacket::PACKET_ID => self.handle_admin_get_user_state(&mut data).await,
            AdminUpdateUserPacket::PACKET_ID => self.handle_admin_update_user(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }

    // packet encoding and sending functions

    /// fast packet sending with best-case zero heap allocation. requires the packet to implement `StaticSize`.
    /// if the packet size isn't known at compile time, derive/implement `DynamicSize` and use `send_packet_dynamic` instead.
    async fn send_packet_static<P: Packet + Encodable + StaticSize>(&self, packet: &P) -> Result<()> {
        // in theory, the size is known at compile time, so we could use a stack array here, instead of using alloca.
        // however in practice, the performance difference is negligible, so we avoid code unnecessary code repetition.
        self.send_packet_alloca(packet, P::ENCODED_SIZE).await
    }

    /// version of `send_packet_static` that does not require the size to be known at compile time.
    /// you are still required to derive/implement `DynamicSize` so the size can be computed at runtime.
    async fn send_packet_dynamic<P: Packet + Encodable + DynamicSize>(&self, packet: &P) -> Result<()> {
        self.send_packet_alloca(packet, packet.encoded_size()).await
    }

    /// use alloca to encode the packet on the stack, and try a non-blocking send, on failure clones to a Vec and a blocking send.
    /// be very careful if using this directly, miscalculating the size may cause a runtime panic.
    #[inline]
    async fn send_packet_alloca<P: Packet + Encodable>(&self, packet: &P, packet_size: usize) -> Result<()> {
        self.send_packet_alloca_with::<P, _>(packet_size, |buf| {
            buf.write_value(packet);
        })
        .await
    }

    /// low level version of other `send_packet_xxx` methods. there is no bound for `Encodable`, `StaticSize` or `DynamicSize`,
    /// but you have to provide a closure that handles packet encoding, and you must specify the appropriate packet size (upper bound).
    /// you do **not** have to encode the packet header or include its size in the `packet_size` argument, that will be done for you automatically.
    #[inline]
    async fn send_packet_alloca_with<P: Packet, F>(&self, packet_size: usize, encode_fn: F) -> Result<()>
    where
        F: FnOnce(&mut FastByteBuffer),
    {
        if cfg!(debug_assertions) && P::PACKET_ID != KeepaliveResponsePacket::PACKET_ID {
            self.print_packet::<P>(true, Some(if P::ENCRYPTED { "fast + encrypted" } else { "fast" }));
        }

        if P::ENCRYPTED {
            // gs_inline_encode! doesn't work here because the borrow checker is silly :(
            let header_start = if P::SHOULD_USE_TCP { size_of_types!(u32) } else { 0usize };

            let nonce_start = header_start + PacketHeader::SIZE;
            let mac_start = nonce_start + NONCE_SIZE;
            let raw_data_start = mac_start + MAC_SIZE;
            let total_size = raw_data_start + packet_size;

            gs_alloca_check_size!(total_size);

            let to_send: Result<Option<Vec<u8>>> = gs_with_alloca!(total_size, data, {
                let mut buf = FastByteBuffer::new(data);

                if P::SHOULD_USE_TCP {
                    // reserve space for packet length
                    buf.write_u32(0);
                }

                // write the header
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
                #[cfg(not(rust_analyzer))] // don't ask about it
                data[nonce_start..mac_start].copy_from_slice(&nonce);

                // prepend the mac tag
                data[mac_start..raw_data_start].copy_from_slice(&tag);

                if P::SHOULD_USE_TCP {
                    // write total packet length
                    let packet_len = (raw_data_end - header_start) as u32;
                    data[..size_of_types!(u32)].copy_from_slice(&packet_len.to_be_bytes());
                }

                // we try a non-blocking send if we can, otherwise fallback to a Vec<u8> and an async send
                let send_data = &data[..raw_data_end];

                let res = if P::SHOULD_USE_TCP {
                    self.send_buffer_tcp_immediate(send_data)
                } else {
                    self.send_buffer_udp_immediate(send_data)
                };

                match res {
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(send_data.to_vec())),
                    Err(e) => Err(e),
                    Ok(written) => {
                        if written == raw_data_end {
                            Ok(None)
                        } else {
                            // send leftover data
                            Ok(Some(data[written..raw_data_end].to_vec()))
                        }
                    }
                }
            });

            if let Some(to_send) = to_send? {
                if P::SHOULD_USE_TCP {
                    self.send_buffer_tcp(&to_send).await?;
                } else {
                    self.send_buffer_udp(&to_send).await?;
                }
            }
        } else {
            let prefix_sz = if P::SHOULD_USE_TCP { size_of_types!(u32) } else { 0usize };

            gs_inline_encode!(self, prefix_sz + PacketHeader::SIZE + packet_size, buf, P::SHOULD_USE_TCP, {
                buf.write_packet_header::<P>();
                encode_fn(&mut buf);
            });
        }

        if P::SHOULD_USE_TCP {
            unsafe {
                self.socket.get_mut().flush().await?;
            }
        }

        Ok(())
    }
}
