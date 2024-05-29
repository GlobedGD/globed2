use std::{
    collections::VecDeque,
    net::{SocketAddr, SocketAddrV4},
    sync::{atomic::Ordering, Arc},
    time::Duration,
};

use globed_shared::{
    anyhow::{self, anyhow, bail},
    crypto_box::{aead::OsRng, PublicKey, SecretKey},
    esp::ByteBufferExtWrite as _,
    logger::*,
    SyncMutex, UserEntry,
};
use rustc_hash::FxHashMap;

#[allow(unused_imports)]
use tokio::sync::oneshot; // no way

use tokio::net::{TcpListener, UdpSocket};

use crate::{
    bridge::{self, CentralBridge},
    client::{thread::ClientThreadOutcome, unauthorized::UnauthorizedThread, ClientThread, ServerThreadMessage, UnauthorizedThreadOutcome},
    data::*,
    state::ServerState,
};

const INLINE_BUFFER_SIZE: usize = 164;
const MAX_UDP_PACKET_SIZE: usize = 65536;
const LARGE_BUFFER_SIZE: usize = 2usize.pow(19); // 2^19, 0.5mb

enum EitherClientThread {
    Authorized(Arc<ClientThread>),
    Unauthorized(Arc<UnauthorizedThread>),
    None,
}

pub struct GameServer {
    pub state: ServerState,
    pub tcp_socket: TcpListener,
    pub udp_socket: UdpSocket,
    /// map udp peer : thread
    pub clients: SyncMutex<FxHashMap<SocketAddrV4, Arc<ClientThread>>>,
    pub unauthorized_clients: SyncMutex<VecDeque<Arc<UnauthorizedThread>>>,
    pub unclaimed_threads: SyncMutex<VecDeque<Arc<ClientThread>>>,
    pub secret_key: SecretKey,
    pub public_key: PublicKey,
    pub bridge: CentralBridge,
    pub standalone: bool,
    pub large_packet_buffer: SyncMutex<Box<[u8]>>,
}

impl GameServer {
    pub fn new(tcp_socket: TcpListener, udp_socket: UdpSocket, state: ServerState, bridge: CentralBridge, standalone: bool) -> Self {
        let secret_key = SecretKey::generate(&mut OsRng);
        let public_key = secret_key.public_key();

        Self {
            state,
            tcp_socket,
            udp_socket,
            clients: SyncMutex::new(FxHashMap::default()),
            unauthorized_clients: SyncMutex::new(VecDeque::new()),
            unclaimed_threads: SyncMutex::new(VecDeque::new()),
            secret_key,
            public_key,
            bridge,
            standalone,
            large_packet_buffer: SyncMutex::new(vec![0; LARGE_BUFFER_SIZE].into_boxed_slice()),
        }
    }

    pub async fn run(&'static self) -> ! {
        info!(
            "Server launched on {} (version: {})",
            self.tcp_socket.local_addr().unwrap(),
            env!("CARGO_PKG_VERSION")
        );

        self.state.room_manager.set_game_server(self);

        // spawn central conf refresher (runs every 5 minutes)
        if !self.standalone {
            tokio::spawn(async {
                let mut interval = tokio::time::interval(Duration::from_mins(5));
                interval.tick().await;

                loop {
                    interval.tick().await;
                    match self.refresh_bootdata().await {
                        Ok(()) => debug!("refreshed central server configuration"),
                        Err(e) => error!("failed to refresh configuration from the central server: {e}"),
                    }
                }
            });

            // spawn the role info refresher as well (slightly less common)
            tokio::spawn(async move {
                let mut interval = tokio::time::interval(Duration::from_mins(30));
                interval.tick().await;

                loop {
                    interval.tick().await;
                    let cc = self.bridge.central_conf.lock();
                    self.state.role_manager.refresh_from(&cc);
                }
            });
        }

        // print some useful stats every once in a bit
        let interval = self.bridge.central_conf.lock().status_print_interval;

        if interval != 0 {
            tokio::spawn(async move {
                let mut interval = tokio::time::interval(Duration::from_secs(interval));
                interval.tick().await;

                loop {
                    interval.tick().await;
                    self.print_server_status();
                }
            });
        }

        // spawn the udp packet handler

        tokio::spawn(async move {
            let mut buf = [0u8; MAX_UDP_PACKET_SIZE];

            loop {
                match self.recv_and_handle_udp(&mut buf).await {
                    Ok(()) => {}
                    Err(e) => {
                        warn!("failed to handle udp packet: {e}");
                    }
                }
            }
        });

        loop {
            match self.accept_connection().await {
                Ok(()) => {}
                Err(err) => {
                    let err_string = err.to_string();
                    error!("Failed to accept a connection: {err_string}");
                    // if it's a fd limit issue, sleep until things get better
                    if err_string.contains("Too many open files") {
                        warn!("fd limit exceeded, sleeping for 500ms. server cannot accept any more clients unless the fd limit is raised");
                        tokio::time::sleep(Duration::from_millis(500)).await;
                    }
                }
            }
        }
    }

    async fn accept_connection(&'static self) -> anyhow::Result<()> {
        let (socket, peer) = self.tcp_socket.accept().await?;

        let peer = match peer {
            SocketAddr::V4(x) => x,
            SocketAddr::V6(_) => bail!("rejecting request from ipv6 host"),
        };

        debug!("accepting tcp connection from {peer}");

        let thread = Arc::new(UnauthorizedThread::new(socket, peer, self));

        self.unauthorized_clients.lock().push_back(thread.clone());

        tokio::spawn(self.client_loop(thread, peer));

        Ok(())
    }

    async fn client_loop(&'static self, in_thread: Arc<UnauthorizedThread>, peer: SocketAddrV4) {
        let mut either_thread: EitherClientThread = EitherClientThread::Unauthorized(in_thread);

        loop {
            match &mut either_thread {
                EitherClientThread::Unauthorized(thread) => {
                    let result = thread.run().await;

                    // unauthorized thread has terminated, remove it from the map.
                    {
                        let mut clients = self.unauthorized_clients.lock();
                        let idx = clients.iter().position(|thr| Arc::ptr_eq(thr, thread));
                        let idx = idx.expect("failed to find thread in unauthorized thread list");

                        clients.remove(idx);
                    }

                    // check if the login was successful
                    match result {
                        UnauthorizedThreadOutcome::Terminate => break,
                        UnauthorizedThreadOutcome::Upgrade => {}
                    }

                    // replace `either_thread` with None, take ownership of the thread
                    let thread = match std::mem::replace(&mut either_thread, EitherClientThread::None) {
                        EitherClientThread::Unauthorized(x) => x,
                        _ => unsafe { std::hint::unreachable_unchecked() },
                    };

                    // we are now supposedly the only reference to the thread, so this should always work
                    let thread = Arc::into_inner(thread).expect("failed to unwrap unauthorized thread");

                    // upgrade to an authorized ClientThread and add it into clients map
                    let thread = Arc::new(thread.upgrade());
                    self.clients.lock().insert(
                        unsafe { thread.socket.get() }.udp_peer.expect("upgraded thread has no udp peer assigned"),
                        thread.clone(),
                    );

                    either_thread = EitherClientThread::Authorized(thread);
                }
                EitherClientThread::Authorized(thread) => {
                    let outcome = thread.run().await;

                    // thread has terminated, remove it from the map.

                    {
                        let mut clients = self.clients.lock();
                        // safety: this is pretty unsafe
                        // TODO
                        let udp_peer = unsafe { thread.socket.get() }.udp_peer.expect("no udp peer in established thread");
                        clients.remove(&udp_peer);
                    }

                    // wait until there are no more references to the thread
                    loop {
                        let ref_count = Arc::strong_count(thread);

                        if ref_count == 1 {
                            break;
                        }

                        debug!("waiting for refcount to be 1..");

                        tokio::time::sleep(Duration::from_millis(100)).await;
                    }

                    // check for the result
                    match outcome {
                        ClientThreadOutcome::Terminate => break,
                        ClientThreadOutcome::Disconnect => {}
                    }

                    // replace `either_thread` with None, take ownership of the thread
                    let thread = match std::mem::replace(&mut either_thread, EitherClientThread::None) {
                        EitherClientThread::Authorized(x) => x,
                        _ => unsafe { std::hint::unreachable_unchecked() },
                    };

                    // we are now supposedly the only reference to the thread, so this should always work
                    let thread = Arc::into_inner(thread).expect("failed to unwrap thread");

                    // downgrade back to an unauthorized thread
                    let thread = Arc::new(thread.into_unauthorized());

                    self.unauthorized_clients.lock().push_back(thread.clone());

                    either_thread = EitherClientThread::Unauthorized(thread);
                }
                EitherClientThread::None => unreachable!(),
            }
        }

        let socket = match &either_thread {
            EitherClientThread::Authorized(x) => &x.socket,
            EitherClientThread::Unauthorized(x) => &x.socket,
            EitherClientThread::None => unreachable!(),
        };

        // safety: the thread no longer runs and we are the only ones who can access the socket
        let socket = unsafe { socket.get_mut() };
        let _ = socket.shutdown().await;

        debug!("cleaning up thread: {} (udp peer: {:?})", socket.tcp_peer, socket.udp_peer);
        self.post_disconnect_cleanup(either_thread).await;
    }

    async fn recv_and_handle_udp(&self, buf: &mut [u8]) -> anyhow::Result<()> {
        let (len, peer) = self.udp_socket.recv_from(buf).await?;

        let peer = match peer {
            SocketAddr::V4(x) => x,
            SocketAddr::V6(_) => bail!("rejecting request from ipv6 host"),
        };

        // if it's a ping packet, we can handle it here. otherwise we send it to the appropriate thread.
        if !self.try_udp_handle(&buf[..len], peer).await? {
            let thread = { self.clients.lock().get(&peer).cloned() };
            if let Some(thread) = thread {
                thread
                    .push_new_message(if len <= INLINE_BUFFER_SIZE {
                        let mut inline_buf = [0u8; INLINE_BUFFER_SIZE];
                        inline_buf[..len].clone_from_slice(&buf[..len]);

                        ServerThreadMessage::SmallPacket((inline_buf, len))
                    } else {
                        ServerThreadMessage::Packet(buf[..len].to_vec())
                    })
                    .await;
            }
        }

        Ok(())
    }

    /* various calls for other threads */

    pub fn claim_thread(&self, udp_addr: SocketAddrV4, secret_key: u32) -> bool {
        let thread = self.unauthorized_clients.lock().iter().find(|x| x.secret_key == secret_key).cloned();

        if let Some(thread) = thread {
            thread.claim(udp_addr);
            true
        } else {
            false
        }
    }

    pub async fn broadcast_voice_packet(&self, vpkt: &Arc<VoiceBroadcastPacket>, level_id: LevelId, room_id: u32) {
        self.broadcast_user_message(&ServerThreadMessage::BroadcastVoice(vpkt.clone()), vpkt.player_id, level_id, room_id)
            .await;
    }

    pub async fn broadcast_chat_packet(&self, tpkt: &ChatMessageBroadcastPacket, level_id: LevelId, room_id: u32) {
        self.broadcast_user_message(&ServerThreadMessage::BroadcastText(tpkt.clone()), tpkt.player_id, level_id, room_id)
            .await;
    }

    /// iterate over every player in this list and run F
    #[inline]
    pub fn for_each_player<F, A>(&self, ids: &[i32], f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerAccountData, usize, &mut A) -> bool,
    {
        self.clients
            .lock()
            .values()
            .filter(|thread| ids.contains(&thread.account_id.load(Ordering::Relaxed)))
            .map(|thread| thread.account_data.lock().clone())
            .fold(0, |count, data| count + usize::from(f(&data, count, additional)))
    }

    /// iterate over every authenticated player and run F
    #[inline]
    pub fn for_every_player_preview<F, A>(&self, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerPreviewAccountData, usize, &mut A) -> bool,
    {
        self.clients
            .lock()
            .values()
            .filter(|thr| thr.authenticated())
            .map(|thread| thread.account_data.lock().make_preview())
            .fold(0, |count, preview| count + usize::from(f(&preview, count, additional)))
    }

    #[inline]
    pub fn for_every_player_preview_in_room<F, A>(&self, room_id: u32, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerPreviewAccountData, usize, &mut A) -> bool,
    {
        self.clients
            .lock()
            .values()
            .filter(|thr| thr.authenticated() && thr.room_id.load(Ordering::Relaxed) == room_id)
            .map(|thread| thread.account_data.lock().make_preview())
            .fold(0, |count, preview| count + usize::from(f(&preview, count, additional)))
    }

    #[inline]
    pub fn for_every_room_player_preview<F, A>(&self, room_id: u32, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerRoomPreviewAccountData, usize, &mut A) -> bool,
    {
        self.clients
            .lock()
            .values()
            .filter(|thr| thr.authenticated() && thr.room_id.load(Ordering::Relaxed) == room_id)
            .map(|thread| {
                let mut level_id = thread.level_id.load(Ordering::Relaxed);

                // if they are in editorcollab, show no level
                if is_editorcollab_level(level_id) {
                    level_id = 0;
                }

                thread.account_data.lock().make_room_preview(level_id)
            })
            .fold(0, |count, preview| count + usize::from(f(&preview, count, additional)))
    }

    /// get a list of all authenticated players
    #[inline]
    pub fn get_player_previews_for_inviting(&self) -> Vec<PlayerPreviewAccountData> {
        let player_count = self.clients.lock().len();

        let mut vec = Vec::with_capacity(player_count);

        self.for_every_player_preview(
            |p, _, vec| {
                vec.push(p.clone());
                true
            },
            &mut vec,
        );

        vec
    }

    /// get a list of all players in a room
    #[inline]
    pub fn get_room_player_previews(&self, room_id: u32) -> Vec<PlayerRoomPreviewAccountData> {
        let player_count = self.state.room_manager.with_any(room_id, |room| room.manager.get_total_player_count());

        let mut vec = Vec::with_capacity(player_count);

        self.for_every_room_player_preview(
            room_id,
            |p, _, vec| {
                vec.push(p.clone());
                true
            },
            &mut vec,
        );

        vec
    }

    #[inline]
    pub fn get_player_previews_in_room(&self, room_id: u32) -> Vec<PlayerPreviewAccountData> {
        let player_count = self.state.room_manager.with_any(room_id, |room| room.manager.get_total_player_count());

        let mut vec = Vec::with_capacity(player_count);

        self.for_every_player_preview_in_room(
            room_id,
            |p, _, vec| {
                vec.push(p.clone());
                true
            },
            &mut vec,
        );

        vec
    }

    #[inline]
    pub fn for_every_public_room<F, A>(&self, f: F, additional: &mut A) -> usize
    where
        F: Fn(RoomListingInfo, usize, &mut A) -> bool,
    {
        self.state.room_manager.get_rooms().iter().fold(0, |count, (id, room)| {
            count + usize::from(f(room.get_room_listing_info(*id, self), count, additional))
        })
    }

    #[inline]
    pub fn get_player_account_data(&self, account_id: i32) -> Option<PlayerAccountData> {
        self.clients
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .map(|thr| thr.account_data.lock().clone())
    }

    #[inline]
    pub fn get_player_preview_data(&self, account_id: i32) -> Option<PlayerPreviewAccountData> {
        self.clients
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .map(|thr| thr.account_data.lock().make_preview())
    }

    /// If someone is already logged in under the given account ID, logs them out.
    /// Additionally, blocks until the appropriate cleanup has been done.
    pub async fn check_already_logged_in(&self, account_id: i32) -> anyhow::Result<()> {
        let thread = self
            .clients
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .cloned();

        if let Some(thread) = thread {
            thread
                .push_new_message(ServerThreadMessage::TerminationNotice(FastString::new(
                    "Someone logged into the same account from a different place.",
                )))
                .await;

            let destruction_notify = thread.destruction_notify.clone();
            drop(thread);

            // we want to wait until the player has been removed from any managers and such.

            match tokio::time::timeout(Duration::from_secs(3), destruction_notify.notified()).await {
                Ok(()) => {}
                Err(_) => return Err(anyhow!("timed out waiting for the thread to disconnect")),
            }
        }

        Ok(())
    }

    /// Find a thread by account ID
    pub fn get_user_by_id(&self, account_id: i32) -> Option<Arc<ClientThread>> {
        self.clients
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .cloned()
    }

    /// If the passed string is numeric, tries to find a user by account ID, else by their account name.
    pub fn find_user(&self, name: &str) -> Option<Arc<ClientThread>> {
        self.clients
            .lock()
            .values()
            .find(|thr| {
                // if it's a valid int, assume it's an account ID
                if let Ok(account_id) = name.parse::<i32>() {
                    thr.account_id.load(Ordering::Relaxed) == account_id
                } else {
                    // else assume it's a player name
                    thr.account_data.lock().name.eq_ignore_ascii_case(name)
                }
            })
            .cloned()
    }

    /// Try to find a user by name or account ID, invoke the passed closure, and if it returns `true`,
    /// send a request to the central server to update the account.
    pub async fn find_and_update_user<F: FnOnce(&mut UserEntry) -> bool>(&self, name: &str, f: F) -> anyhow::Result<()> {
        if let Some(thread) = self.find_user(name) {
            self.update_user(&thread, f).await
        } else {
            Err(anyhow!("failed to find the user"))
        }
    }

    pub async fn update_user<F: FnOnce(&mut UserEntry) -> bool>(&self, thread: &ClientThread, f: F) -> anyhow::Result<()> {
        let result = {
            let mut data = thread.user_entry.lock();
            f(&mut data)
        };

        if result {
            let user_entry = thread.user_entry.lock().clone();
            self.bridge.update_user_data(&user_entry).await?;
        }

        Ok(())
    }

    /* private handling stuff */

    /// broadcast a message to all people on the level
    async fn broadcast_user_message(&self, msg: &ServerThreadMessage, origin_id: i32, level_id: LevelId, room_id: u32) {
        let threads = self.state.room_manager.with_any(room_id, |pm| {
            let players = pm.manager.get_level(level_id);

            if let Some(players) = players {
                self.clients
                    .lock()
                    .values()
                    .filter(|thread| {
                        let account_id = thread.account_id.load(Ordering::Relaxed);
                        account_id != origin_id && players.contains(&account_id)
                    })
                    .cloned()
                    .collect()
            } else {
                Vec::new()
            }
        });

        for thread in threads {
            thread.push_new_message(msg.clone()).await;
        }
    }

    /// broadcast a message to all people in a room
    pub async fn broadcast_room_message(&self, msg: &ServerThreadMessage, origin_id: i32, room_id: u32) {
        let threads: Vec<_> = self
            .clients
            .lock()
            .values()
            .filter(|thread| {
                let account_id = thread.account_id.load(Ordering::Relaxed);
                thread.room_id.load(Ordering::Relaxed) == room_id && account_id != 0 && account_id != origin_id
            })
            .cloned()
            .collect();

        for thread in threads {
            thread.push_new_message(msg.clone()).await;
        }
    }

    /// send `RoomInfoPacket` to all players in a room
    pub async fn broadcast_room_info(&self, room_id: u32) {
        if room_id == 0 {
            return;
        }

        let info = self
            .state
            .room_manager
            .try_with_any(room_id, |room| Some(room.get_room_info(room_id, self)), || None);

        if let Some(info) = info {
            let pkt = RoomInfoPacket { info };

            self.broadcast_room_message(&ServerThreadMessage::BroadcastRoomInfo(pkt), 0, room_id)
                .await;
        }
    }

    /// Try to handle a packet that is not addresses to a specific thread, but to the game server.
    async fn try_udp_handle(&self, data: &[u8], peer: SocketAddrV4) -> anyhow::Result<bool> {
        let mut byte_reader = ByteReader::from_bytes(data);
        let header = byte_reader.read_packet_header().map_err(|e| anyhow!("{e}"))?;

        match header.packet_id {
            PingPacket::PACKET_ID => {
                let pkt = PingPacket::decode_from_reader(&mut byte_reader).map_err(|e| anyhow!("{e}"))?;
                let response = PingResponsePacket {
                    id: pkt.id,
                    player_count: self.state.player_count.load(Ordering::Relaxed),
                };

                let mut buf_array = [0u8; PacketHeader::SIZE + PingResponsePacket::ENCODED_SIZE];
                let mut buf = FastByteBuffer::new(&mut buf_array);
                buf.write_packet_header::<PingResponsePacket>();
                buf.write_value(&response);

                let send_bytes = buf.as_bytes();

                match self.udp_socket.try_send_to(send_bytes, SocketAddr::V4(peer)) {
                    Ok(_) => Ok(true),
                    Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                        self.udp_socket.send_to(send_bytes, peer).await?;
                        Ok(true)
                    }
                    Err(e) => Err(e.into()),
                }
            }

            ClaimThreadPacket::PACKET_ID => {
                let pkt = ClaimThreadPacket::decode_from_reader(&mut byte_reader).map_err(|e| anyhow!("{e}"))?;
                if !self.claim_thread(peer, pkt.secret_key) {
                    warn!("udp peer {peer} tried to claim an invalid thread (with key {})", pkt.secret_key);
                }
                Ok(true)
            }

            _ => Ok(false),
        }
    }

    async fn post_disconnect_cleanup(&self, thread: EitherClientThread) {
        let (account_id, level_id, room_id) = match thread {
            EitherClientThread::Authorized(thread) => {
                thread.destruction_notify.notify_one();

                (
                    thread.account_id.load(Ordering::Relaxed),
                    thread.level_id.load(Ordering::Relaxed),
                    thread.room_id.load(Ordering::Relaxed),
                )
            }
            EitherClientThread::Unauthorized(thread) => (
                thread.account_id.load(Ordering::Relaxed),
                thread.level_id.load(Ordering::Relaxed),
                thread.room_id.load(Ordering::Relaxed),
            ),
            EitherClientThread::None => unreachable!(),
        };

        if account_id == 0 {
            return;
        }

        // decrement player count
        self.state.player_count.fetch_sub(1, Ordering::Relaxed);

        // remove from the player manager and the level if they are on one
        let was_owner = self.state.room_manager.remove_with_any(room_id, account_id, level_id);

        // also send room update i guess
        if was_owner && room_id != 0 {
            self.broadcast_room_info(room_id).await;
        }
    }

    fn print_server_status(&self) {
        info!("Current server stats");
        info!(
            "Player count: {} (threads: {}, unclaimed: {})",
            self.state.player_count.load(Ordering::Relaxed),
            self.clients.lock().len(),
            self.unclaimed_threads.lock().len(),
        );
        info!("Amount of rooms: {}", self.state.room_manager.get_rooms().len());
        info!(
            "People in the global room: {}",
            self.state.room_manager.get_global().manager.get_total_player_count()
        );
        info!("-------------------------------------------");
    }

    async fn refresh_bootdata(&self) -> bridge::Result<()> {
        self.bridge.refresh_boot_data().await?;

        // if we are now under maintenance, disconnect everyone who's still connected
        if self.bridge.is_maintenance() {
            let threads: Vec<_> = self.clients.lock().values().cloned().collect();
            for thread in threads {
                thread
                    .push_new_message(ServerThreadMessage::TerminationNotice(FastString::new(
                        "The server is now under maintenance, please try connecting again later",
                    )))
                    .await;
            }
        }

        Ok(())
    }
}
