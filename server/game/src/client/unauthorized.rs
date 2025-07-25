use std::{
    borrow::Cow,
    collections::VecDeque,
    net::SocketAddr,
    sync::{
        Arc,
        atomic::{AtomicBool, AtomicI32, AtomicU16, AtomicU32, Ordering},
    },
    time::Duration,
};

#[allow(unused_imports)]
use globed_shared::{
    MAX_SUPPORTED_PROTOCOL, MIN_CLIENT_VERSION, MIN_SUPPORTED_PROTOCOL, SUPPORTED_PROTOCOLS, SyncMutex, debug, info,
    rand::{self, Rng},
    warn,
};
use globed_shared::{ServerUserEntry, should_ignore_error};

use super::*;
use crate::{
    data::*,
    managers::{ComputedRole, Room},
    server::GameServer,
    tokio::{self, net::TcpStream, sync::Notify},
    util::{LockfreeMutCell, SimpleRateLimiter},
};

/// `UnauthorizedThread` is a thread that can be formed for 2 reasons:
/// 1. Initial connection (when a client initiates a TCP connection, an `UnauthorizedThread` is created)
/// 2. Abrupt disconnect (a `ClientThread` will be downgraded to a `UnauthorizedThread` and can be reclaimed for some time)
///
/// In the first mode, two things can happen:
/// 1. Handshake -> `LoginPacket` -> `ClaimThreadPacket` -> thread gets upgraded to a `ClientThread`
/// 2. `LoginRecoverPacket` -> merge with the found `UnauthorizedThread` -> `ClaimThreadPacket` -> thread gets upgraded
///
/// In the second mode, the server waits for someone to try and recover this thread, while it's in `Disconnected` state.
pub struct UnauthorizedThread {
    pub game_server: &'static GameServer,
    pub socket: LockfreeMutCell<ClientSocket>,
    pub connection_state: AtomicClientThreadState,

    pub secret_key: u32,
    pub protocol_version: AtomicU16,

    pub account_id: AtomicI32,
    pub level_id: AtomicLevelId,
    pub on_unlisted_level: AtomicBool,
    pub room_id: AtomicU32,
    pub link_code: AtomicU32,
    pub room: SyncMutex<Arc<Room>>,

    pub account_data: SyncMutex<PlayerAccountData>,
    pub user_entry: SyncMutex<Option<ServerUserEntry>>,
    pub user_role: SyncMutex<Option<ComputedRole>>,
    pub friend_list: SyncMutex<Vec<i32>>,

    pub claim_udp_peer: SyncMutex<Option<SocketAddr>>,
    pub claim_udp_notify: Notify,
    pub claim_udp_none: AtomicBool,
    pub queued_packets: SyncMutex<VecDeque<Vec<u8>>>,

    pub recover_stream: SyncMutex<Option<(TcpStream, SocketAddr)>>,
    pub recover_notify: Notify,

    pub terminate_notify: Notify,

    pub destruction_notify: Arc<Notify>,
    pub translator: NoopPacketTranslator,

    pub privacy_settings: SyncMutex<UserPrivacyFlags>,

    // private fields to unauth thread, not used when promoting
    rate_limiter: LockfreeMutCell<SimpleRateLimiter>,
}

pub enum UnauthorizedThreadOutcome {
    Upgrade,
    Terminate,
}

const TIMEOUT: Duration = Duration::from_secs(90);

fn create_rate_limiter() -> SimpleRateLimiter {
    SimpleRateLimiter::new(10, Duration::from_millis(900))
}

impl UnauthorizedThread {
    pub fn new(socket: TcpStream, peer: SocketAddr, game_server: &'static GameServer) -> Self {
        Self {
            game_server,
            socket: LockfreeMutCell::new(ClientSocket::new(socket, peer, 0, game_server)),
            connection_state: AtomicClientThreadState::default(),

            secret_key: rand::rng().random(),
            protocol_version: AtomicU16::new(0),

            account_id: AtomicI32::new(0),
            level_id: AtomicLevelId::new(0),
            on_unlisted_level: AtomicBool::new(false),
            room_id: AtomicU32::new(0),
            link_code: AtomicU32::new(0),
            room: SyncMutex::new(game_server.state.room_manager.get_global_owned()),

            account_data: SyncMutex::new(PlayerAccountData::default()),
            user_entry: SyncMutex::new(None),
            user_role: SyncMutex::new(None),
            friend_list: SyncMutex::new(Vec::new()),

            claim_udp_peer: SyncMutex::new(None),
            claim_udp_notify: Notify::new(),
            claim_udp_none: AtomicBool::new(false),
            queued_packets: SyncMutex::new(VecDeque::new()),

            recover_stream: SyncMutex::new(None),
            recover_notify: Notify::new(),

            terminate_notify: Notify::new(),

            destruction_notify: Arc::new(Notify::new()),
            translator: NoopPacketTranslator::default(),

            privacy_settings: SyncMutex::new(UserPrivacyFlags::default()),
            rate_limiter: LockfreeMutCell::new(create_rate_limiter()),
        }
    }

    pub fn downgrade(thread: ClientThread) -> Self {
        Self {
            game_server: thread.game_server,
            socket: thread.socket,
            connection_state: AtomicClientThreadState::new(ClientThreadState::Disconnected),

            secret_key: thread.secret_key,
            protocol_version: thread.protocol_version,

            account_id: thread.account_id,
            level_id: thread.level_id,
            on_unlisted_level: thread.on_unlisted_level,
            room_id: thread.room_id,
            link_code: thread.link_code,
            room: thread.room,

            account_data: SyncMutex::new(std::mem::take(&mut *thread.account_data.lock())),
            user_entry: SyncMutex::new(Some(std::mem::take(&mut *thread.user_entry.lock()))),
            user_role: SyncMutex::new(Some(std::mem::take(&mut *thread.user_role.lock()))),
            friend_list: SyncMutex::new(std::mem::take(&mut *thread.friend_list.lock())),

            claim_udp_peer: SyncMutex::new(None),
            claim_udp_notify: Notify::new(),
            claim_udp_none: AtomicBool::new(false),
            queued_packets: SyncMutex::new(std::mem::take(&mut *thread.queued_packets.lock())),

            recover_stream: SyncMutex::new(None),
            recover_notify: Notify::new(),

            terminate_notify: Notify::new(),

            destruction_notify: thread.destruction_notify,
            translator: NoopPacketTranslator::default(),

            privacy_settings: thread.privacy_settings,
            rate_limiter: LockfreeMutCell::new(create_rate_limiter()),
        }
    }

    /// Returns whether the thread should be upgraded.
    pub async fn run(&self) -> UnauthorizedThreadOutcome {
        loop {
            let state = self.connection_state.load();

            match state {
                ClientThreadState::Established => break UnauthorizedThreadOutcome::Upgrade,
                ClientThreadState::Terminating => break UnauthorizedThreadOutcome::Terminate,

                /* disconnected state, wait until another tcp stream tries to recover us */
                ClientThreadState::Disconnected => tokio::select! {
                    x = tokio::time::timeout(TIMEOUT, self.wait_for_recovered()) => match x {
                        Ok((stream, tcp_peer)) => {
                            // we just got recovered yay
                            let socket = self.get_socket();

                            #[cfg(debug_assertions)]
                            debug!(
                                "recovered thread ({}), previous peer: {}, current: {}",
                                self.account_id.load(Ordering::Relaxed),
                                socket.tcp_peer,
                                tcp_peer
                            );

                            socket.socket = stream;
                            socket.tcp_peer = tcp_peer;

                            if let Err(e) = self.send_login_success().await {
                                thrd_warn!(self, "failed to send login success: {e}");
                                self.terminate();
                                continue;
                            }

                            self.connection_state.store(ClientThreadState::Unclaimed);
                        }
                        Err(_) => {
                            // we did not get recovered in the given time, terminate
                            self.terminate();
                        }
                    },

                    () = self.wait_for_termianted() => {
                        self.terminate();
                    }
                },

                /* unauthorized state, wait until the user sends a handshake and a LoginPacket */
                ClientThreadState::Unauthorized => tokio::select! {
                    x = tokio::time::timeout(TIMEOUT, self.get_socket().poll_for_tcp_data()) => match x {
                        Ok(Ok(datalen)) => match self.recv_and_handle(datalen).await {
                            Ok(()) => {}
                            Err(e) => {
                                thrd_warn!(self, "error on an unauth thread: {e}");
                                #[cfg(debug_assertions)]
                                let _ = self.terminate_with_message(Cow::Owned(format!("failed to authenticate: {e}"))).await;
                                #[cfg(not(debug_assertions))]
                                let _ = self.terminate_with_message(Cow::Borrowed("internal server error during player authentication")).await;
                            }
                        },

                        Ok(Err(err)) => {
                            // terminate, an error occurred

                            // ignore certain IO errors
                            if let PacketHandlingError::IOError(ref e) = err && should_ignore_error(e) {
                                #[cfg(debug_assertions)]
                                thrd_debug!(self, "fatal error on an unauth thread, terminating: {err}");
                            } else {
                                thrd_warn!(self, "fatal error on an unauth thread, terminating: {err}");
                            }
                            self.terminate();
                        }

                        Err(_) => {
                            // time is up, call quits
                            self.terminate();
                        }
                    },

                    () = self.wait_for_termianted() => {
                        self.terminate();
                    }
                },

                /* unclaimed state, wait until user sends a ClaimThreadPacket and gameserver notifies us */
                ClientThreadState::Unclaimed => tokio::select! {
                    biased; // we want to prioritize termination & wait_for_claimed in case the user sends another packet right after

                    () = self.wait_for_termianted() => {
                        self.terminate();
                    }

                    x = tokio::time::timeout(TIMEOUT, self.wait_for_claimed()) => match x {
                        Ok(()) => {
                            // we just got claimed, we can leave and upgrade into a ClientThread
                            self.connection_state.store(ClientThreadState::Established);
                        }

                        Err(_) => {
                            // time is up, call quits
                            self.terminate();
                        }
                    },

                    // alternatively, wait until they send us a SkipClaimThreadPacket
                    x = tokio::time::timeout(TIMEOUT, self.get_socket().poll_for_tcp_data()) => match x {
                        Ok(Ok(datalen)) => match self.recv_and_handle(datalen).await {
                            Ok(()) => {}
                            Err(e) => {
                                thrd_warn!(self, "error on an unauth thread: {e}");
                                #[cfg(debug_assertions)]
                                let _ = self.terminate_with_message(Cow::Owned(format!("failed to authenticate: {e}"))).await;
                                #[cfg(not(debug_assertions))]
                                let _ = self.terminate_with_message(Cow::Borrowed("internal server error during player authentication")).await;
                            }
                        },

                        Ok(Err(err)) => {
                            // terminate, an error occurred

                            // ignore certain IO errors
                            if let PacketHandlingError::IOError(ref e) = err && should_ignore_error(e) {
                                #[cfg(debug_assertions)]
                                thrd_debug!(self, "fatal error on an unauth thread, terminating: {err}");
                            } else {
                                thrd_warn!(self, "fatal error on an unauth thread, terminating: {err}");
                            }
                            self.terminate();
                        }

                        Err(_) => {
                            self.terminate();
                        }
                    },
                },
            }
        }
    }

    pub fn claim(&self, udp_peer: SocketAddr) {
        *self.claim_udp_peer.lock() = Some(udp_peer);
        self.claim_udp_notify.notify_one();
    }

    pub fn recover(&self, tcp_stream: TcpStream, peer: SocketAddr) {
        *self.recover_stream.lock() = Some((tcp_stream, peer));
        self.recover_notify.notify_one();
    }

    #[inline]
    async fn recv_and_handle(&self, message_size: usize) -> Result<()> {
        // safety: only we can receive data from our client.
        let socket = self.get_socket();
        socket.recv_and_handle(message_size, async |buf| self.handle_packet(buf).await).await
    }

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

        if self.connection_state.load() == ClientThreadState::Unclaimed && header.packet_id != SkipClaimThreadPacket::PACKET_ID {
            #[cfg(debug_assertions)]
            thrd_debug!(self, "received packet {} while in unclaimed state, saving it for later", header.packet_id);

            self.queued_packets.lock().push_back(message.to_vec());

            return Ok(());
        }

        // reject cleartext credentials
        if header.packet_id == LoginPacket::PACKET_ID && !header.encrypted {
            return Err(PacketHandlingError::MalformedLoginAttempt);
        }

        // decrypt the packet in-place if encrypted
        if header.encrypted {
            data = self.get_socket().decrypt(message)?;
        }

        match header.packet_id {
            CryptoHandshakeStartPacket::PACKET_ID => self.handle_crypto_handshake(&mut data).await,
            PingPacket::PACKET_ID => self.handle_ping(&mut data).await,
            LoginPacket::PACKET_ID => self.handle_login(&mut data).await,
            ConnectionTestPacket::PACKET_ID => self.handle_connection_test(&mut data).await,
            SkipClaimThreadPacket::PACKET_ID => self.handle_skip_claim_thread(&mut data).await,
            UpdateFriendListPacket::PACKET_ID => self.handle_update_friend_list(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }

    // packet handlers

    gs_handler!(self, handle_ping, PingPacket, packet, {
        let socket = self.get_socket();

        socket
            .send_packet_static(&PingResponsePacketTCP {
                id: packet.id,
                player_count: self.game_server.state.get_player_count(),
            })
            .await?;

        return Ok(());
    });

    gs_handler!(self, handle_crypto_handshake, CryptoHandshakeStartPacket, packet, {
        let socket = self.get_socket();

        if !SUPPORTED_PROTOCOLS.contains(&packet.protocol) && packet.protocol != 0xffff {
            self.terminate();

            socket
                .send_packet_dynamic(&ProtocolMismatchPacket {
                    // send minimum required version
                    protocol: MIN_SUPPORTED_PROTOCOL,
                    min_client_version: Cow::Borrowed(MIN_CLIENT_VERSION),
                })
                .await?;

            return Ok(());
        }

        socket.set_protocol_version(packet.protocol);
        self.protocol_version.store(packet.protocol, Ordering::Relaxed);

        socket.init_crypto_box(&packet.key)?;
        socket
            .send_packet_static(&CryptoHandshakeResponsePacket {
                key: self.game_server.public_key.clone().into(),
            })
            .await
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        // preemptively set the status to terminating, in case anything fails later.
        // if login was successful, change the status back at the end of the method body.
        self.terminate();

        let socket = self.get_socket();

        // disconnect if server is under maintenance
        if self.game_server.bridge.central_conf.lock().maintenance {
            gs_disconnect!(self, "The server is currently under maintenance, please try connecting again later.");
        }

        if packet.fragmentation_limit < 1300 {
            gs_disconnect!(
                self,
                format!(
                    "The client fragmentation limit is too low ({} bytes) to be accepted",
                    packet.fragmentation_limit
                )
            );
        }

        unsafe { self.socket.get_mut().set_mtu(packet.fragmentation_limit as usize) };

        if packet.account_id <= 0 || packet.user_id <= 0 {
            let message = Cow::Owned(format!(
                "Invalid account/user ID was sent ({} and {}). Please note that you must be signed into a Geometry Dash account before connecting.",
                packet.account_id, packet.user_id
            ));

            socket.send_packet_dynamic(&LoginFailedPacket { message }).await?;
            return Ok(());
        }

        // skip authentication if standalone
        let standalone = self.game_server.standalone;
        let player_name = if standalone {
            packet.name
        } else {
            // lets verify the given token
            let result = {
                self.game_server
                    .bridge
                    .token_issuer
                    .lock()
                    .validate(packet.account_id, packet.user_id, packet.token.to_str().unwrap())
            };

            match result {
                Ok(x) => InlineString::new(&x),
                Err(err) => {
                    let mut message = FastString::new("authentication failed: ");
                    message.extend(err.error_message());

                    socket
                        .send_packet_dynamic(&LoginFailedPacket {
                            message: Cow::Owned(message.to_string()),
                        })
                        .await?;
                    return Ok(());
                }
            }
        };

        // check if the user is already logged in, kick the other instance
        self.game_server.check_already_logged_in(packet.account_id).await?;

        // fetch data from the central
        if !standalone {
            let response = match self.game_server.bridge.user_login(packet.account_id, &player_name).await {
                Ok(response) if response.ban.is_some() => {
                    let ban = response.ban.unwrap();

                    socket
                        .send_packet_dynamic(&ServerBannedPacket {
                            message: FastString::new(if ban.reason.is_empty() { "No reason given" } else { &ban.reason }),
                            expires_at: ban.expires_at as u64,
                        })
                        .await?;

                    return Ok(());
                }
                Ok(response) if self.game_server.bridge.is_whitelist() && !response.user_entry.is_whitelisted => {
                    socket
                        .send_packet_dynamic(&LoginFailedPacket {
                            message: Cow::Borrowed("This server has whitelist enabled and your account has not been allowed."),
                        })
                        .await?;

                    return Ok(());
                }
                Ok(response) => response,
                Err(err) => {
                    let mut message = InlineString::<256>::new("failed to fetch user data: ");
                    message.extend_safe(&err.to_string());

                    socket
                        .send_packet_dynamic(&LoginFailedPacket {
                            message: Cow::Owned(message.to_string()),
                        })
                        .await?;
                    return Ok(());
                }
            };

            *self.user_role.lock() = Some(self.game_server.state.role_manager.compute(&response.user_entry.user_roles));
            *self.user_entry.lock() = Some(response.user_entry);
            self.link_code.store(response.link_code, Ordering::Relaxed);
        }

        self.account_id.store(packet.account_id, Ordering::Relaxed);
        self.game_server.state.inc_player_count(); // increment player count

        info!(
            "[{} ({}) @ {}] Login successful, platform: {}, protocol v{}",
            player_name,
            packet.account_id,
            self.get_tcp_peer(),
            packet.platform,
            self.protocol_version.load(Ordering::Relaxed)
        );

        {
            let mut account_data = self.account_data.lock();
            account_data.account_id = packet.account_id;
            account_data.user_id = packet.user_id;
            account_data.name = player_name;
            account_data.icons.clone_from(&packet.icons);

            let user_entry = self.user_entry.lock();
            if let Some(user_entry) = &*user_entry {
                let sud = SpecialUserData::from_roles(&user_entry.user_roles, &self.game_server.state.role_manager);

                account_data.special_user_data = sud;
            }
        };

        // add them to the global room
        self.game_server
            .state
            .room_manager
            .get_global()
            .manager
            .write()
            .create_player(packet.account_id, packet.privacy_settings.get_hide_in_game());

        self.send_login_success().await?;

        self.connection_state.store(ClientThreadState::Unclaimed); // as we still need ClaimThreadPacket to arrive
        self.privacy_settings.lock().clone_from(&packet.privacy_settings);

        Ok(())
    });

    gs_handler!(self, handle_connection_test, ConnectionTestPacket, packet, {
        let socket = self.get_socket();

        socket
            .send_packet_dynamic(&ConnectionTestResponsePacket {
                uid: packet.uid,
                data: packet.data,
            })
            .await
    });

    gs_handler!(self, handle_skip_claim_thread, SkipClaimThreadPacket, _packet, {
        self.claim_udp_none.store(true, Ordering::SeqCst);
        self.claim_udp_notify.notify_one();

        Ok(())
    });

    gs_handler!(self, handle_update_friend_list, UpdateFriendListPacket, packet, {
        let mut friend_list = self.friend_list.lock();
        friend_list.clear();

        for id in &packet.ids {
            friend_list.push(*id);
        }

        // sort
        friend_list.sort_unstable();

        Ok(())
    });

    async fn send_login_success(&self) -> Result<()> {
        let tps = self.game_server.bridge.central_conf.lock().tps;

        let all_roles = self.game_server.state.role_manager.get_all_roles();
        let special_user_data = self.account_data.lock().special_user_data.clone();

        let socket = self.get_socket();
        let client_protocol = self.protocol_version.load(Ordering::Relaxed);

        socket
            .send_packet_dynamic(&LoggedInPacket {
                tps,
                all_roles,
                secret_key: self.secret_key,
                special_user_data,
                server_protocol: client_protocol,
            })
            .await
    }

    /// Blocks until we get notified that we got claimed by a UDP socket.
    async fn wait_for_claimed(&self) {
        {
            let mut p = self.claim_udp_peer.lock();
            if p.is_some() {
                self.get_socket().udp_peer = std::mem::take(&mut *p);
                return;
            } else if self.claim_udp_none.load(Ordering::SeqCst) {
                // if we are not expecting a UDP peer, just return
                return;
            }
        }

        loop {
            self.claim_udp_notify.notified().await;

            let mut p = self.claim_udp_peer.lock();
            if p.is_some() {
                self.get_socket().udp_peer = std::mem::take(&mut *p);
                return;
            } else if self.claim_udp_none.load(Ordering::SeqCst) {
                // if we are not expecting a UDP peer, just return
                return;
            }
        }
    }

    /// Blocks until we get notified that we got recovered and have an assigned TCP stream
    async fn wait_for_recovered(&self) -> (TcpStream, SocketAddr) {
        {
            let mut p = self.recover_stream.lock();
            if p.is_some() {
                return p.take().unwrap();
            }
        }

        loop {
            self.recover_notify.notified().await;

            let mut p = self.recover_stream.lock();
            if p.is_some() {
                return p.take().unwrap();
            }
        }
    }

    /// Blocks until we get notified that our thread is being terminated
    async fn wait_for_termianted(&self) {
        self.terminate_notify.notified().await;
    }

    fn terminate(&self) {
        self.connection_state.store(ClientThreadState::Terminating);
    }

    async fn terminate_with_message(&self, message: Cow<'_, str>) -> Result<()> {
        self.get_socket().send_packet_dynamic(&ServerDisconnectPacket { message }).await?;

        self.terminate();

        Ok(())
    }

    pub fn request_termination(&self) {
        self.terminate_notify.notify_one();
    }

    /// do not call from another thread unless this thread is NOT running.
    /// otherwise always invokes undefined behavior.
    #[allow(clippy::mut_from_ref)]
    fn get_socket(&self) -> &mut ClientSocket {
        unsafe { self.socket.get_mut() }
    }

    /// get the tcp address of the connected peer. do not call this from another clientthread
    fn get_tcp_peer(&self) -> SocketAddr {
        self.get_socket().tcp_peer
    }

    /// terminate and send a message to the user with the reason
    async fn kick(&self, message: Cow<'_, str>) -> Result<()> {
        self.terminate();
        self.get_socket().send_packet_dynamic(&ServerDisconnectPacket { message }).await
    }

    pub fn upgrade(self) -> ClientThread {
        // make a couple of assertions that must always hold true before upgrading

        debug_assert!(
            self.get_socket().udp_peer.is_some() || self.claim_udp_none.load(Ordering::SeqCst),
            "udp peer is None when upgrading thread, and skip claim was not set"
        );
        debug_assert!(
            self.account_id.load(std::sync::atomic::Ordering::Relaxed) != 0,
            "account ID is 0 when upgrading thread"
        );
        debug_assert!(
            self.connection_state.load() == ClientThreadState::Established,
            "connection state is not Established when upgrading thread"
        );

        ClientThread::from_unauthorized(self)
    }
}
