use std::{
    collections::VecDeque,
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU16, AtomicU32, Ordering},
        Arc,
    },
    time::Duration,
};

use esp::ByteReader;
use globed_shared::{logger::*, SyncMutex, UserEntry};
use handlers::game::MAX_VOICE_PACKET_SIZE;
use tokio::{
    io::AsyncReadExt,
    net::TcpStream,
    sync::{Mutex, Notify},
};

use crate::{
    data::*,
    managers::ComputedRole,
    server::GameServer,
    util::{LockfreeMutCell, SimpleRateLimiter},
};

pub use super::*;

pub mod handlers;

pub const INLINE_BUFFER_SIZE: usize = 164;

const MAX_PACKET_SIZE: usize = 65536;

#[derive(Clone)]
pub enum ServerThreadMessage {
    SmallPacket(([u8; INLINE_BUFFER_SIZE], usize)),
    Packet(Vec<u8>),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    BroadcastText(ChatMessageBroadcastPacket),
    BroadcastNotice(ServerNoticePacket),
    BroadcastInvite(RoomInvitePacket),
    BroadcastRoomInfo(RoomInfoPacket),
    BroadcastBan(ServerBannedPacket),
    BroadcastMute(ServerMutedPacket),
    BroadcastRoleChange(RolesUpdatedPacket),
    TerminationNotice(FastString),
}

pub struct ClientThread {
    game_server: &'static GameServer,
    pub socket: LockfreeMutCell<ClientSocket>,

    pub claimed: AtomicBool,
    pub claim_secret_key: AtomicU32,

    awaiting_termination: AtomicBool,
    pub is_authorized_admin: AtomicBool,
    pub account_id: AtomicI32,
    pub level_id: AtomicLevelId,
    pub room_id: AtomicU32,
    pub account_data: SyncMutex<PlayerAccountData>,
    pub user_entry: SyncMutex<UserEntry>,
    pub user_role: SyncMutex<ComputedRole>,

    pub cleanup_notify: Notify,
    pub cleanup_mutex: Mutex<()>,
    fragmentation_limit: AtomicU16,

    message_queue: Mutex<VecDeque<ServerThreadMessage>>,
    message_notify: Notify,
    rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
    voice_rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
    chat_rate_limiter: Option<LockfreeMutCell<SimpleRateLimiter>>,
}

impl ClientThread {
    /* public api for the main server */

    pub fn new(socket: TcpStream, peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        let (rate_limiter, voice_rate_limiter, chat_rate_limiter) = {
            let conf = game_server.bridge.central_conf.lock();

            (
                SimpleRateLimiter::new(conf.tps as usize + 6, Duration::from_millis(900)),
                SimpleRateLimiter::new(5, Duration::from_millis(1000)),
                if conf.chat_burst_interval != 0 && conf.chat_burst_limit != 0 {
                    Some(SimpleRateLimiter::new(
                        conf.chat_burst_limit as usize,
                        Duration::from_millis(u64::from(conf.chat_burst_interval)),
                    ))
                } else {
                    None
                },
            )
        };

        Self {
            game_server,
            socket: LockfreeMutCell::new(ClientSocket::new(socket, peer, game_server)),
            claimed: AtomicBool::new(false),
            claim_secret_key: AtomicU32::new(0),
            is_authorized_admin: AtomicBool::new(false),
            account_id: AtomicI32::new(0),
            level_id: AtomicLevelId::new(0),
            room_id: AtomicU32::new(0),
            awaiting_termination: AtomicBool::new(false),
            account_data: SyncMutex::new(PlayerAccountData::default()),
            user_entry: SyncMutex::new(UserEntry::default()),
            user_role: SyncMutex::new(game_server.state.role_manager.get_default().clone()),
            cleanup_notify: Notify::new(),
            cleanup_mutex: Mutex::new(()),
            fragmentation_limit: AtomicU16::new(0),
            message_queue: Mutex::new(VecDeque::new()),
            message_notify: Notify::new(),
            rate_limiter: LockfreeMutCell::new(rate_limiter),
            voice_rate_limiter: LockfreeMutCell::new(voice_rate_limiter),
            chat_rate_limiter: chat_rate_limiter.map(LockfreeMutCell::new),
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
        let sock = unsafe { self.socket.get_mut() };
        sock.poll_for_tcp_data().await
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
                        // try to stop silly HTTP request attempts
                        // 0x47455420 is the ascii string "GET " and 0x504f5354 is "POST"
                        if message_len == 0x47_45_54_20 || message_len == 0x50_4f_53_54 {
                            warn!("[{}] received an unexpected (potential) HTTP request, disconnecting", self.get_tcp_peer());
                            break;
                        }

                        match self.recv_and_handle(message_len).await {
                            Ok(()) => {}
                            Err(e) => self.print_error(&e),
                        }
                    }
                    Ok(Err(err)) => {
                        // early eof means the client has disconnected (closed their part of the socket)
                        if let PacketHandlingError::IOError(ref ioerror) = err {
                            if ioerror.kind() != std::io::ErrorKind::UnexpectedEof {
                                self.print_error(&err);
                            }
                        }
                        break;
                    }
                    Err(_) => {
                        debug!("tcp connection not responding for 90 seconds, disconnecting");
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

    /// get the tcp address of the connected peer. do not call this from another clientthread
    fn get_tcp_peer(&self) -> SocketAddrV4 {
        unsafe { self.socket.get() }.tcp_peer
    }

    // the error printing is different in release and debug. some errors have higher severity than others.
    fn print_error(&self, error: &PacketHandlingError) {
        if cfg!(debug_assertions) {
            warn!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
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
                    warn!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
                }
                // these are either our fault or a fatal error somewhere
                PacketHandlingError::ColorParseFailed(_)
                | PacketHandlingError::UnexpectedCentralResponse
                | PacketHandlingError::SystemTimeError(_)
                | PacketHandlingError::WebRequestError(_)
                | PacketHandlingError::DangerousAllocation(_)
                | PacketHandlingError::UnableToSendUdp => {
                    error!("[{} @ {}] err: {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
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

    /// call `self.terminate()` and send a message to the user with the reason
    async fn disconnect(&self, message: &str) -> Result<()> {
        self.terminate();
        self.send_packet_dynamic(&ServerDisconnectPacket { message }).await
    }

    async fn ban(&self, message: FastString, timestamp: i64) -> Result<()> {
        self.terminate();
        self.send_packet_dynamic(&ServerBannedPacket { message, timestamp }).await
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

        // check for slowmode stuffs
        if voice {
            if len > MAX_VOICE_PACKET_SIZE {
                // voice packet is too big
                return false;
            }

            // safety: only we can access the rate limiters of our user.
            let block = !unsafe { self.voice_rate_limiter.get_mut().try_tick() };
            if block {
                return false;
            }
        } else {
            // if rate limiting is disabled, do not block
            let block = !self.chat_rate_limiter.as_ref().map_or(true, |x| unsafe { x.get_mut().try_tick() });

            if block {
                return false;
            }
        }

        true
    }

    async fn recv_and_handle(&self, message_size: usize) -> Result<()> {
        // safety: only we can receive data from our client.
        if message_size > MAX_PACKET_SIZE {
            return Err(PacketHandlingError::PacketTooLong(message_size));
        }

        let socket = unsafe { self.socket.get_mut() };

        let data: &mut [u8];

        let mut inline_buf = [0u8; INLINE_BUFFER_SIZE];
        let mut heap_buf = Vec::<u8>::new();

        let use_inline = message_size <= INLINE_BUFFER_SIZE;

        if use_inline {
            data = &mut inline_buf[..message_size];
            socket.socket.read_exact(data).await?;
        } else {
            let sock = &mut socket.socket;

            let read_bytes = sock.take(message_size as u64).read_to_end(&mut heap_buf).await?;
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
                self.send_packet_dynamic(&packet).await?;
                info!("{} is receiving a notice: {}", self.account_data.lock().name, packet.message);
            }
            ServerThreadMessage::BroadcastInvite(packet) => self.send_packet_static(&packet).await?,
            ServerThreadMessage::BroadcastRoomInfo(packet) => {
                self.send_packet_static(&packet).await?;
            }
            ServerThreadMessage::BroadcastBan(packet) => self.ban(packet.message, packet.timestamp).await?,
            ServerThreadMessage::BroadcastMute(packet) => self.send_packet_dynamic(&packet).await?,
            ServerThreadMessage::BroadcastRoleChange(packet) => self.send_packet_static(&packet).await?,
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
            data = unsafe { self.socket.get_mut() }.decrypt(message)?;
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

            /* room related */
            CreateRoomPacket::PACKET_ID => self.handle_create_room(&mut data).await,
            JoinRoomPacket::PACKET_ID => self.handle_join_room(&mut data).await,
            LeaveRoomPacket::PACKET_ID => self.handle_leave_room(&mut data).await,
            RequestRoomPlayerListPacket::PACKET_ID => self.handle_request_room_players(&mut data).await,
            UpdateRoomSettingsPacket::PACKET_ID => self.handle_update_room_settings(&mut data).await,
            RoomSendInvitePacket::PACKET_ID => self.handle_room_invitation(&mut data).await,
            RequestRoomListPacket::PACKET_ID => self.handle_request_room_list(&mut data).await,

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

    #[inline]
    async fn send_packet_static<P: Packet + Encodable + StaticSize>(&self, packet: &P) -> Result<()> {
        unsafe { self.socket.get_mut() }.send_packet_static(packet).await
    }

    #[inline]
    async fn send_packet_dynamic<P: Packet + Encodable + DynamicSize>(&self, packet: &P) -> Result<()> {
        unsafe { self.socket.get_mut() }.send_packet_dynamic(packet).await
    }

    #[inline]
    #[allow(unused)]
    async fn send_packet_alloca<P: Packet + Encodable>(&self, packet: &P, packet_size: usize) -> Result<()> {
        unsafe { self.socket.get_mut() }.send_packet_alloca(packet, packet_size).await
    }

    #[inline]
    async fn send_packet_alloca_with<P: Packet, F>(&self, packet_size: usize, encode_fn: F) -> Result<()>
    where
        F: FnOnce(&mut FastByteBuffer),
    {
        unsafe { self.socket.get_mut() }
            .send_packet_alloca_with::<P, F>(packet_size, encode_fn)
            .await
    }
}
