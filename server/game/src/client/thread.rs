use std::{
    borrow::Cow,
    collections::VecDeque,
    net::SocketAddr,
    sync::{
        Arc,
        atomic::{AtomicBool, AtomicI32, AtomicU16, AtomicU32, Ordering},
    },
    time::{Duration, SystemTime},
};

use crate::{
    managers::Room,
    tokio::{
        self,
        sync::{Mutex, Notify},
    },
};
use esp::ByteReader;
use globed_shared::{
    ServerUserEntry, SyncMutex,
    logger::*,
    rand::{self, RngCore},
    should_ignore_error,
};
use handlers::game::MAX_VOICE_PACKET_SIZE;
use slotmap::DefaultKey;
use tokio::time::Instant;

use crate::{
    data::*,
    managers::ComputedRole,
    server::GameServer,
    util::{LockfreeMutCell, SimpleRateLimiter},
};

use self::socket::ProtocolOverride;

pub use super::*;

pub mod handlers;

pub const INLINE_BUFFER_SIZE: usize = 164;
pub const THREAD_MICRO_TIMEOUT: Duration = Duration::from_secs(30);

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
    BroadcastNoticeReply(PendingNoticeReply),
    BroadcastRoomKicked,
    TerminationNotice(FastString),
}

#[derive(Clone)]
pub struct PendingNoticeReply {
    pub reply_id: u32,          // unique id of the notice reply
    pub moderator_id: i32,      // account id of the moderator
    pub user_id: i32,           // account id of the target user
    pub admin_msg: FastString,  // message that was sent by the admin
    pub user_reply: FastString, // the reply of the user
    pub sent_at: SystemTime,
}

impl Default for PendingNoticeReply {
    fn default() -> Self {
        Self {
            reply_id: 0,
            moderator_id: 0,
            user_id: 0,
            admin_msg: FastString::default(),
            user_reply: FastString::default(),
            sent_at: SystemTime::now(),
        }
    }
}

pub struct ClientThread {
    pub game_server: &'static GameServer,
    pub socket: LockfreeMutCell<ClientSocket>,
    connection_state: AtomicClientThreadState,

    pub secret_key: u32,
    pub protocol_version: AtomicU16,

    pub account_id: AtomicI32,
    pub level_id: AtomicLevelId,
    pub on_unlisted_level: AtomicBool,
    pub room_id: AtomicU32,
    pub link_code: AtomicU32,
    pub room: SyncMutex<Arc<Room>>,

    pub account_data: SyncMutex<PlayerAccountData>,
    pub user_entry: SyncMutex<ServerUserEntry>,
    pub user_role: SyncMutex<ComputedRole>,
    pub friend_list: SyncMutex<Vec<i32>>,

    pub queued_packets: SyncMutex<VecDeque<Vec<u8>>>,

    pub is_authorized_user: AtomicBool,

    pub privacy_settings: SyncMutex<UserPrivacyFlags>,

    message_queue: Mutex<VecDeque<ServerThreadMessage>>,
    message_notify: Notify,
    rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
    voice_rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
    chat_rate_limiter: Option<LockfreeMutCell<SimpleRateLimiter>>,
    translator: PacketTranslator,
    slotmap_key: SyncMutex<Option<DefaultKey>>,
    pub pending_notice_replies: SyncMutex<Vec<PendingNoticeReply>>,

    pub destruction_notify: Arc<Notify>,
}

pub enum ClientThreadOutcome {
    Terminate,  // complete termination
    Disconnect, // downgrade to unauthorized thread, allow the user to reconnect
}

impl ClientThread {
    pub fn from_unauthorized(thread: UnauthorizedThread) -> Self {
        let game_server = thread.game_server;

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

        let account_data = std::mem::take(&mut *thread.account_data.lock());
        let user_entry = std::mem::take(&mut *thread.user_entry.lock()).unwrap_or_default();
        let user_role = std::mem::take(&mut *thread.user_role.lock()).unwrap_or_else(|| game_server.state.role_manager.get_default().clone());
        let friend_list = std::mem::take(&mut *thread.friend_list.lock());
        let queued_packets = std::mem::take(&mut *thread.queued_packets.lock());
        let translator = PacketTranslator::new(thread.protocol_version.load(Ordering::Relaxed));

        Self {
            game_server,
            socket: thread.socket,
            connection_state: thread.connection_state,

            secret_key: thread.secret_key,
            protocol_version: thread.protocol_version,

            account_id: thread.account_id,
            level_id: thread.level_id,
            on_unlisted_level: thread.on_unlisted_level,
            room_id: thread.room_id,
            link_code: thread.link_code,
            room: thread.room,

            account_data: SyncMutex::new(account_data),
            user_entry: SyncMutex::new(user_entry),
            user_role: SyncMutex::new(user_role),
            friend_list: SyncMutex::new(friend_list),

            queued_packets: SyncMutex::new(queued_packets),

            is_authorized_user: AtomicBool::new(false),

            privacy_settings: thread.privacy_settings,

            message_queue: Mutex::new(VecDeque::new()),
            message_notify: Notify::new(),
            rate_limiter: LockfreeMutCell::new(rate_limiter),
            voice_rate_limiter: LockfreeMutCell::new(voice_rate_limiter),
            chat_rate_limiter: chat_rate_limiter.map(LockfreeMutCell::new),
            translator,
            slotmap_key: SyncMutex::new(None),
            pending_notice_replies: SyncMutex::new(Vec::new()),

            destruction_notify: thread.destruction_notify,
        }
    }

    pub fn into_unauthorized(self) -> UnauthorizedThread {
        UnauthorizedThread::downgrade(self)
    }

    /* public api for the main server */

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
        // safety: we trust this function is not called from the oustide
        let sock = unsafe { self.socket.get_mut() };
        sock.poll_for_tcp_data().await
    }

    pub async fn run(&self) -> ClientThreadOutcome {
        let mut last_received_packet = Instant::now();

        {
            loop {
                let pkt = self.queued_packets.lock().pop_front();

                match pkt {
                    Some(pkt) => {
                        if let Err(err) = self.handle_message(ServerThreadMessage::Packet(pkt)).await {
                            self.print_error(&err);
                        }
                    }
                    None => break,
                }
            }
        }

        loop {
            let state = self.connection_state.load();

            match state {
                ClientThreadState::Disconnected | ClientThreadState::Unauthorized => break ClientThreadOutcome::Disconnect,
                ClientThreadState::Terminating => break ClientThreadOutcome::Terminate,
                ClientThreadState::Unclaimed => unreachable!(),
                ClientThreadState::Established => {}
            }

            // if more than 90 seconds since last packet, disonnect
            if last_received_packet.elapsed().as_secs() > 90 {
                break self.terminate();
            }

            tokio::select! {
                message = self.poll_for_messages() => {
                    if let Some(message) = message {
                        match message {
                            ServerThreadMessage::Packet(_) | ServerThreadMessage::SmallPacket(_) => {
                                // update last received packet
                                last_received_packet = Instant::now();
                            },
                            _ => {}
                        }

                        match self.handle_message(message).await {
                            Ok(()) => {}
                            Err(e) => self.print_error(&e),
                        }
                    }
                }

                tcp_res = self.poll_for_tcp_data() => match tcp_res {
                    Ok(message_len) => {
                        // try to stop silly HTTP request attempts
                        // 0x47455420 is the ascii string "GET " and 0x504f5354 is "POST"
                        if message_len == 0x47_45_54_20 || message_len == 0x50_4f_53_54 {
                            warn!("[{}] received an unexpected (potential) HTTP request, disconnecting", self.get_tcp_peer());
                            break self.terminate();
                        }

                        // update last received packet
                        last_received_packet = Instant::now();

                        match self.recv_and_handle(message_len).await {
                            Ok(()) => {}
                            Err(e) => self.print_error(&e),
                        }
                    }
                    Err(err) => {
                        self.print_error(&err);
                        break self.disconnect();
                    }
                },

                () = tokio::time::sleep(THREAD_MICRO_TIMEOUT) => {
                    continue;
                }
            };
        }
    }

    pub fn authenticated(&self) -> bool {
        self.account_id.load(Ordering::Relaxed) != 0
    }

    // shorthand for checking if user has mod perms, and is authorized
    pub fn can_moderate(&self) -> bool {
        self.is_authorized_user.load(Ordering::Relaxed) && self.user_role.lock().can_moderate()
    }

    pub fn is_in_room(&self) -> bool {
        self.room_id.load(Ordering::Relaxed) != 0
    }

    /// schedule the thread to terminate as soon as possible.
    #[inline]
    pub fn terminate(&self) -> ClientThreadOutcome {
        self.connection_state.store(ClientThreadState::Terminating);
        ClientThreadOutcome::Terminate
    }

    /// schedule the thread to terminate as soon as possible, but allowing reconnects
    #[inline]
    pub fn disconnect(&self) -> ClientThreadOutcome {
        self.connection_state.store(ClientThreadState::Disconnected);
        ClientThreadOutcome::Disconnect
    }

    pub async fn push_new_message(&self, message: ServerThreadMessage) {
        self.message_queue.lock().await.push_back(message);
        self.message_notify.notify_one();
    }

    /* private utilities */

    /// get the tcp address of the connected peer. do not call this from another clientthread
    fn get_tcp_peer(&self) -> SocketAddr {
        // safety: we trust this function is not called from the oustide
        unsafe { self.socket.get() }.tcp_peer
    }

    // the error printing is different in release and debug. some errors have higher severity than others.
    fn print_error(&self, error: &PacketHandlingError) {
        if cfg!(debug_assertions) {
            warn!("[{} @ {}] {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
        } else {
            match error {
                // these are for the client being silly
                PacketHandlingError::Other(_)
                | PacketHandlingError::WrongCryptoBoxState
                | PacketHandlingError::EncryptionError
                | PacketHandlingError::DecryptionError
                | PacketHandlingError::NoHandler(_)
                | PacketHandlingError::DebugOnlyPacket
                | PacketHandlingError::PacketTooLong(_)
                | PacketHandlingError::SocketSendFailed(_)
                | PacketHandlingError::InvalidStreamMarker
                | PacketHandlingError::TooManyChunks(_)
                | PacketHandlingError::TranslationError(_) => {
                    warn!("[{} @ {}] {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
                }

                PacketHandlingError::IOError(e) => {
                    if !should_ignore_error(e) {
                        warn!("[{} @ {}] {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), e);
                    }
                }
                // these are either our fault or a fatal error somewhere
                PacketHandlingError::ColorParseFailed(_)
                | PacketHandlingError::UnexpectedCentralResponse
                | PacketHandlingError::SystemTimeError(_)
                | PacketHandlingError::WebRequestError(_)
                | PacketHandlingError::DangerousAllocation(_)
                | PacketHandlingError::UnableToSendUdp
                | PacketHandlingError::BridgeError(_)
                | PacketHandlingError::WebhookError(_) => {
                    error!("[{} @ {}] {}", self.account_id.load(Ordering::Relaxed), self.get_tcp_peer(), error);
                }
                // these can likely never happen unless network corruption or someone is pentesting, so ignore in release
                PacketHandlingError::MalformedMessage
                | PacketHandlingError::MalformedCiphertext
                | PacketHandlingError::MalformedLoginAttempt
                | PacketHandlingError::MalformedPacketStructure(_)
                | PacketHandlingError::SocketWouldBlock
                | PacketHandlingError::Ratelimited
                | PacketHandlingError::UnexpectedPlayerData
                | PacketHandlingError::NoPermission
                | PacketHandlingError::Standalone => {}
            }
        }
    }

    /// call `self.terminate()` and send a message to the user with the reason
    async fn kick(&self, message: Cow<'_, str>) -> Result<()> {
        self.terminate();
        self.send_packet_dynamic(&ServerDisconnectPacket { message }).await
    }

    async fn ban(&self, message: FastString, expires_at: u64) -> Result<()> {
        self.terminate();
        self.send_packet_dynamic(&ServerBannedPacket { message, expires_at }).await
    }

    fn is_chat_packet_allowed(&self, voice: bool, len: usize) -> bool {
        let accid = self.account_id.load(Ordering::Relaxed);
        if accid == 0 {
            // unauthorized
            return false;
        }

        if self.user_entry.lock().active_mute.is_some() {
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

    pub fn create_notice_reply(&self, moderator_id: i32, admin_msg: FastString) -> u32 {
        let user_id = self.account_id.load(Ordering::Relaxed);
        let reply_id = rand::rng().next_u32();

        let reply = PendingNoticeReply {
            admin_msg,
            user_id,
            reply_id,
            moderator_id,
            user_reply: FastString::default(),
            sent_at: SystemTime::now(),
        };

        self.pending_notice_replies.lock().push(reply);

        reply_id
    }

    pub fn set_slotmap_key(&self, key: DefaultKey) {
        *self.slotmap_key.lock() = Some(key);
    }

    pub fn take_slotmap_key(&self) -> Option<DefaultKey> {
        let mut k = self.slotmap_key.lock();
        k.take()
    }

    pub fn routine_cleanup(&self) {
        // delete all pending replies that are over 1 hour old
        self.pending_notice_replies
            .lock()
            .retain(|reply| reply.sent_at.elapsed().unwrap_or_default() < Duration::from_hours(1));
    }

    #[inline]
    async fn recv_and_handle(&self, message_size: usize) -> Result<()> {
        // safety: only we can receive data from our client.
        let socket = unsafe { self.socket.get_mut() };
        socket.recv_and_handle(message_size, async |buf| self.handle_packet(buf).await).await
    }

    /// handle a message sent from the `GameServer`
    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(mut packet) => self.handle_packet(&mut packet).await?,
            ServerThreadMessage::SmallPacket((mut packet, len)) => self.handle_packet(&mut packet[..len]).await?,
            ServerThreadMessage::BroadcastText(text_packet) => self.send_packet_static(&text_packet).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => {
                let tcp = self.privacy_settings.lock().get_tcp_audio();

                if tcp {
                    self.send_packet_dynamic_tcp(&*voice_packet).await?
                } else {
                    self.send_packet_dynamic(&*voice_packet).await?
                }
            }
            ServerThreadMessage::BroadcastNotice(packet) => {
                info!("{} is receiving a notice: {}", self.account_data.lock().name, packet.message);
                self.send_packet_translatable(packet).await?;
            }
            ServerThreadMessage::BroadcastInvite(packet) => {
                let invitable = !self.privacy_settings.lock().get_no_invites();

                if invitable {
                    self.send_packet_static(&packet).await?;
                }
            }
            ServerThreadMessage::BroadcastRoomInfo(packet) => {
                self.send_packet_static(&packet).await?;
            }
            ServerThreadMessage::BroadcastBan(packet) => self.ban(packet.message, packet.expires_at).await?,
            ServerThreadMessage::BroadcastMute(packet) => self.send_packet_dynamic(&packet).await?,
            ServerThreadMessage::BroadcastRoleChange(packet) => self.send_packet_static(&packet).await?,
            ServerThreadMessage::BroadcastNoticeReply(reply) => {
                let user_name = if let Some(user) = self.game_server.get_user_by_id(reply.user_id) {
                    FastString::new(user.account_data.lock().name.try_to_str())
                } else {
                    FastString::new("<unknown>")
                };

                info!(
                    "{user_name} ({}) sent a notice reply to {}: \"{}\"",
                    reply.user_id, reply.moderator_id, reply.user_reply
                );

                self.send_packet_dynamic(&AdminReceivedNoticeReplyPacket {
                    reply_id: reply.reply_id,
                    user_id: reply.user_id,
                    user_name,
                    admin_msg: reply.admin_msg,
                    user_reply: reply.user_reply,
                })
                .await?;
            }
            ServerThreadMessage::BroadcastRoomKicked => self._kicked_from_room().await?,
            ServerThreadMessage::TerminationNotice(message) => self.kick(Cow::Borrowed(message.try_to_str())).await?,
        }

        Ok(())
    }

    /// handle an incoming packet
    async fn handle_packet(&self, message: &mut [u8]) -> Result<()> {
        #[cfg(debug_assertions)]
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

            let user_muted = self.user_entry.lock().active_mute.is_some();

            // XXX: protocol compat
            if self.protocol_version.load(Ordering::Relaxed) >= 14 {
                return self.send_packet_static(&VoiceFailedPacket { user_muted }).await;
            } else {
                return Ok(());
            }
        }

        // decrypt the packet in-place if encrypted
        if header.encrypted {
            data = unsafe { self.socket.get_mut().decrypt(message)? };
        }

        match header.packet_id {
            /* connection related */
            PingPacket::PACKET_ID => self.handle_ping(&mut data).await,
            KeepalivePacket::PACKET_ID => self.handle_keepalive(&mut data).await,
            DisconnectPacket::PACKET_ID => self.handle_disconnect(&mut data),
            ConnectionTestPacket::PACKET_ID => self.handle_connection_test(&mut data).await,
            KeepaliveTCPPacket::PACKET_ID => self.handle_keepalive_tcp(&mut data).await,

            /* general */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestGlobalPlayerListPacket::PACKET_ID => self.handle_request_global_list(&mut data).await,
            RequestLevelListPacket::PACKET_ID => self.handle_request_level_list(&mut data).await,
            RequestPlayerCountPacket::PACKET_ID => self.handle_request_player_count(&mut data).await,
            UpdatePlayerStatusPacket::PACKET_ID => self.handle_set_player_status(&mut data).await,
            LinkCodeRequestPacket::PACKET_ID => self.handle_link_code_request(&mut data).await,
            RequestMotdPacket::PACKET_ID => self.handle_motd_request(&mut data).await,
            UpdateFriendListPacket::PACKET_ID => self.handle_update_friend_list(&mut data).await,

            /* game related */
            RequestPlayerProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,
            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            ChatMessagePacket::PACKET_ID => self.handle_chat_message(&mut data).await,
            NoticeReplyPacket::PACKET_ID => self.handle_notice_reply(&mut data).await,

            /* room related */
            CreateRoomPacket::PACKET_ID => self.handle_create_room(&mut data).await,
            JoinRoomPacket::PACKET_ID => self.handle_join_room(&mut data).await,
            LeaveRoomPacket::PACKET_ID => self.handle_leave_room(&mut data).await,
            RequestRoomPlayerListPacket::PACKET_ID => self.handle_request_room_players(&mut data).await,
            UpdateRoomSettingsPacket::PACKET_ID => self.handle_update_room_settings(&mut data).await,
            RoomSendInvitePacket::PACKET_ID => self.handle_room_invitation(&mut data).await,
            RequestRoomListPacket::PACKET_ID => self.handle_request_room_list(&mut data).await,
            CloseRoomPacket::PACKET_ID => self.handle_close_room(&mut data).await,
            KickRoomPlayerPacket::PACKET_ID => self.handle_kick_room_player(&mut data).await,

            /* admin related */
            AdminAuthPacket::PACKET_ID => self.handle_admin_auth(&mut data).await,
            AdminSendNoticePacket::PACKET_ID => self.handle_admin_send_notice(&mut data).await,
            AdminDisconnectPacket::PACKET_ID => self.handle_admin_disconnect(&mut data).await,
            AdminGetUserStatePacket::PACKET_ID => self.handle_admin_get_user_state(&mut data).await,
            AdminSendFeaturedLevelPacket::PACKET_ID => self.handle_admin_send_featured_level(&mut data).await,

            AdminUpdateUsernamePacket::PACKET_ID => self.handle_admin_update_username(&mut data).await,
            AdminSetNameColorPacket::PACKET_ID => self.handle_admin_set_name_color(&mut data).await,
            AdminSetUserRolesPacket::PACKET_ID => self.handle_admin_set_user_roles(&mut data).await,
            AdminPunishUserPacket::PACKET_ID => self.handle_admin_punish_user(&mut data).await,
            AdminRemovePunishmentPacket::PACKET_ID => self.handle_admin_remove_punishment(&mut data).await,
            AdminWhitelistPacket::PACKET_ID => self.handle_admin_whitelist(&mut data).await,
            AdminSetAdminPasswordPacket::PACKET_ID => self.handle_admin_set_admin_password(&mut data).await,
            AdminEditPunishmentPacket::PACKET_ID => self.handle_admin_edit_punishment(&mut data).await,
            AdminGetPunishmentHistoryPacket::PACKET_ID => self.handle_admin_get_punishment_history(&mut data).await,

            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }

    // packet encoding and sending functions

    #[inline]
    fn proto_override(&self) -> ProtocolOverride {
        if unsafe { self.socket.get() }.udp_peer.is_some() {
            ProtocolOverride::None
        } else {
            ProtocolOverride::Tcp // tcp if the user is in tcp-only mode
        }
    }

    #[inline]
    async fn send_packet_static<P: Packet + Encodable + StaticSize>(&self, packet: &P) -> Result<()> {
        unsafe { self.socket.get_mut() }
            .send_packet_static_override(packet, self.proto_override())
            .await
    }

    #[inline]
    async fn send_packet_dynamic<P: Packet + Encodable + DynamicSize>(&self, packet: &P) -> Result<()> {
        unsafe { self.socket.get_mut() }
            .send_packet_dynamic_override(packet, self.proto_override())
            .await
    }

    #[inline]
    async fn send_packet_dynamic_tcp<P: Packet + Encodable + DynamicSize>(&self, packet: &P) -> Result<()> {
        unsafe { self.socket.get_mut() }
            .send_packet_dynamic_override(packet, ProtocolOverride::Tcp)
            .await
    }

    #[inline]
    async fn send_packet_translatable<P: Packet + Encodable + DynamicSize + PartialTranslatableEncodable>(&self, packet: P) -> Result<()> {
        unsafe { self.socket.get_mut() }
            .send_packet_translatable(packet, self.proto_override())
            .await
    }

    #[inline]
    #[allow(unused)]
    async fn send_packet_alloca<P: Packet + Encodable>(&self, packet: &P, packet_size: usize) -> Result<()> {
        unsafe { self.socket.get_mut() }
            .send_packet_alloca(packet, packet_size, self.proto_override())
            .await
    }

    #[inline]
    async fn send_packet_alloca_with<P: Packet, F>(&self, packet_size: usize, encode_fn: F) -> Result<()>
    where
        F: FnOnce(&mut FastByteBuffer),
    {
        unsafe { self.socket.get_mut() }
            .send_packet_alloca_with::<P, F>(packet_size, self.proto_override(), encode_fn)
            .await
    }
}
