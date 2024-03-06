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

use tokio::{
    io::AsyncWriteExt,
    net::{TcpListener, UdpSocket},
};

use crate::{
    bridge::{self, CentralBridge},
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage, INLINE_BUFFER_SIZE},
    state::ServerState,
};

const MAX_UDP_PACKET_SIZE: usize = 65536;

pub struct GameServer {
    pub state: ServerState,
    pub tcp_socket: TcpListener,
    pub udp_socket: UdpSocket,
    pub threads: SyncMutex<FxHashMap<SocketAddrV4, Arc<GameServerThread>>>,
    pub unclaimed_threads: SyncMutex<VecDeque<Arc<GameServerThread>>>,
    pub secret_key: SecretKey,
    pub public_key: PublicKey,
    pub bridge: CentralBridge,
    pub standalone: bool,
}

impl GameServer {
    pub fn new(
        tcp_socket: TcpListener,
        udp_socket: UdpSocket,
        state: ServerState,
        bridge: CentralBridge,
        standalone: bool,
    ) -> Self {
        let secret_key = SecretKey::generate(&mut OsRng);
        let public_key = secret_key.public_key();

        Self {
            state,
            tcp_socket,
            udp_socket,
            threads: SyncMutex::new(FxHashMap::default()),
            unclaimed_threads: SyncMutex::new(VecDeque::new()),
            secret_key,
            public_key,
            bridge,
            standalone,
        }
    }

    pub async fn run(&'static self) -> ! {
        info!(
            "Server launched on {} (version: {})",
            self.tcp_socket.local_addr().unwrap(),
            env!("CARGO_PKG_VERSION")
        );

        // spawn central conf refresher (runs every 5 minutes)
        if !self.standalone {
            tokio::spawn(async {
                let mut interval = tokio::time::interval(Duration::from_secs(300));
                interval.tick().await;

                loop {
                    interval.tick().await;
                    match self.refresh_bootdata().await {
                        Ok(()) => debug!("refreshed central server configuration"),
                        Err(e) => error!("failed to refresh configuration from the central server: {e}"),
                    }
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
                        tokio::time::sleep(Duration::from_millis(250)).await;
                        warn!("fd limit exceeded, sleeping for 250ms.");
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

        let thread = Arc::new(GameServerThread::new(socket, peer, self));
        self.unclaimed_threads.lock().push_back(thread.clone());

        tokio::spawn(async move {
            // `thread.run()` will return in either one of those 3 conditions:
            // 1. no messages sent by the peer for 60 seconds
            // 2. the channel was closed (normally impossible for that to happen)
            // 3. `thread.terminate()` was called on that thread (due to a disconnect from either side)
            // additionally, if it panics then the state of the player will be frozen forever,
            // they won't be removed from levels or the player count and that person has to restart the game to connect again.
            // so try to avoid panics please..
            thread.run().await;
            debug!("removing client: {}", peer);
            self.post_disconnect_cleanup(&thread, peer);

            // safety: the thread no longer runs and we are the only ones who can access the socket
            let socket = unsafe { thread.socket.get_mut() };
            let _ = socket.shutdown().await;

            // if any thread was waiting for us to terminate, tell them it's finally time.
            thread.cleanup_notify.notify_one();
        });

        Ok(())
    }

    async fn recv_and_handle_udp(&'static self, buf: &mut [u8]) -> anyhow::Result<()> {
        let (len, peer) = self.udp_socket.recv_from(buf).await?;

        let peer = match peer {
            SocketAddr::V4(x) => x,
            SocketAddr::V6(_) => bail!("rejecting request from ipv6 host"),
        };

        // if it's a ping packet, we can handle it here. otherwise we send it to the appropriate thread.
        if !self.try_udp_handle(&buf[..len], peer).await? {
            let thread = { self.threads.lock().get(&peer).cloned() };
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

    pub fn claim_thread(&'static self, udp_addr: SocketAddrV4, secret_key: u32) -> bool {
        let mut unclaimed = self.unclaimed_threads.lock();
        let idx = unclaimed.iter().position(|thr| {
            thr.claim_secret_key.load(Ordering::Relaxed) == secret_key && !thr.claimed.load(Ordering::Relaxed)
        });

        if let Some(idx) = idx {
            if let Some(thread) = unclaimed.remove(idx) {
                *thread.udp_peer.lock() = udp_addr;
                thread.claimed.store(true, Ordering::Relaxed);
                self.threads.lock().insert(udp_addr, thread);

                return true;
            }
        }

        false
    }

    pub async fn broadcast_voice_packet(&'static self, vpkt: &Arc<VoiceBroadcastPacket>, level_id: LevelId, room_id: u32) {
        self.broadcast_user_message(
            &ServerThreadMessage::BroadcastVoice(vpkt.clone()),
            vpkt.player_id,
            level_id,
            room_id,
        )
        .await;
    }

    pub async fn broadcast_chat_packet(&'static self, tpkt: &ChatMessageBroadcastPacket, level_id: LevelId, room_id: u32) {
        self.broadcast_user_message(
            &ServerThreadMessage::BroadcastText(tpkt.clone()),
            tpkt.player_id,
            level_id,
            room_id,
        )
        .await;
    }

    /// iterate over every player in this list and run F
    pub fn for_each_player<F, A>(&'static self, ids: &[i32], f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerAccountData, usize, &mut A) -> bool,
    {
        self.threads
            .lock()
            .values()
            .filter(|thread| ids.contains(&thread.account_id.load(Ordering::Relaxed)))
            .map(|thread| thread.account_data.lock().clone())
            .fold(0, |count, data| count + usize::from(f(&data, count, additional)))
    }

    /// iterate over every authenticated player and run F
    pub fn for_every_player_preview<F, A>(&'static self, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerPreviewAccountData, usize, &mut A) -> bool,
    {
        self.threads
            .lock()
            .values()
            .filter(|thr| thr.authenticated())
            .map(|thread| thread.account_data.lock().make_preview())
            .fold(0, |count, preview| count + usize::from(f(&preview, count, additional)))
    }

    pub fn for_every_room_player_preview<F, A>(&'static self, room_id: u32, f: F, additional: &mut A) -> usize
    where
        F: Fn(&PlayerRoomPreviewAccountData, usize, &mut A) -> bool,
    {
        self.threads
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

    pub fn get_player_account_data(&'static self, account_id: i32) -> Option<PlayerAccountData> {
        self.threads
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .map(|thr| thr.account_data.lock().clone())
    }

    /// If someone is already logged in under the given account ID, logs them out.
    /// Additionally, blocks until the appropriate cleanup has been done.
    pub async fn check_already_logged_in(&'static self, user_id: i32) -> anyhow::Result<()> {
        let thread = self
            .threads
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == user_id)
            .cloned();

        if let Some(thread) = thread {
            thread
                .push_new_message(ServerThreadMessage::TerminationNotice(FastString::from_str(
                    "Someone logged into the same account from a different place.",
                )))
                .await;

            let fut = async move {
                let _ = thread.cleanup_mutex.lock().await;
                thread.cleanup_notify.notified().await;
            };

            match tokio::time::timeout(Duration::from_secs(10), fut).await {
                Ok(()) => {}
                Err(_) => return Err(anyhow!("timed out waiting for the thread to disconnect")),
            }
        }

        Ok(())
    }

    /// Find a thread by account ID
    pub fn get_user_by_id(&'static self, account_id: i32) -> Option<Arc<GameServerThread>> {
        self.threads
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == account_id)
            .cloned()
    }

    /// If the passed string is numeric, tries to find a user by account ID, else by their account name.
    pub fn find_user(&'static self, name: &str) -> Option<Arc<GameServerThread>> {
        self.threads
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
    pub async fn find_and_update_user<F: FnOnce(&mut UserEntry) -> bool>(
        &'static self,
        name: &str,
        f: F,
    ) -> anyhow::Result<()> {
        if let Some(thread) = self.find_user(name) {
            self.update_user(&thread, f).await
        } else {
            Err(anyhow!("failed to find the user"))
        }
    }

    pub async fn update_user<F: FnOnce(&mut UserEntry) -> bool>(
        &'static self,
        thread: &GameServerThread,
        f: F,
    ) -> anyhow::Result<()> {
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
    async fn broadcast_user_message(
        &'static self,
        msg: &ServerThreadMessage,
        origin_id: i32,
        level_id: LevelId,
        room_id: u32,
    ) {
        let threads = self.state.room_manager.with_any(room_id, |pm| {
            let players = pm.get_level(level_id);

            if let Some(players) = players {
                self.threads
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

    /// Try to handle a packet that is not addresses to a specific thread, but to the game server.
    async fn try_udp_handle(&'static self, data: &[u8], peer: SocketAddrV4) -> anyhow::Result<bool> {
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
                    warn!(
                        "udp peer {peer} tried to claim an invalid thread (with key {})",
                        pkt.secret_key
                    );
                }
                Ok(true)
            }

            _ => Ok(false),
        }
    }

    fn post_disconnect_cleanup(&'static self, thread: &Arc<GameServerThread>, tcp_peer: SocketAddrV4) {
        if thread.claimed.load(Ordering::Relaxed) {
            // self.threads.lock().retain(|_udp_peer, thread| thread.tcp_peer != tcp_peer);
            let mut threads = self.threads.lock();
            let udp_peer = threads
                .iter()
                .find(|(_udp, thread)| thread.tcp_peer == tcp_peer)
                .map(|x| *x.0);

            if let Some(udp_peer) = udp_peer {
                threads.remove(&udp_peer);
            }
        } else {
            let mut unclaimed = self.unclaimed_threads.lock();
            let idx = unclaimed.iter().position(|thr| Arc::ptr_eq(thr, thread));
            if let Some(idx) = idx {
                unclaimed.remove(idx);
            }
        }

        let account_id = thread.account_id.load(Ordering::Relaxed);

        if account_id == 0 {
            return;
        }

        let level_id = thread.level_id.load(Ordering::Relaxed);
        let room_id = thread.room_id.load(Ordering::Relaxed);

        // decrement player count
        self.state.player_count.fetch_sub(1, Ordering::Relaxed);

        // remove from the player manager and the level if they are on one

        self.state.room_manager.with_any(room_id, |pm| {
            pm.remove_player(account_id);

            if level_id != 0 {
                pm.remove_from_level(level_id, account_id);
            }
        });

        if room_id != 0 {
            self.state.room_manager.maybe_remove_room(room_id);
        }
    }

    fn print_server_status(&'static self) {
        info!("Current server stats");
        info!(
            "Player count: {} (threads: {}, unclaimed: {})",
            self.state.player_count.load(Ordering::Relaxed),
            self.threads.lock().len(),
            self.unclaimed_threads.lock().len(),
        );
        info!("Amount of rooms: {}", self.state.room_manager.get_rooms().len());
        info!(
            "People in the global room: {}",
            self.state.room_manager.get_global().get_total_player_count()
        );
        info!("-------------------------------------------");
    }

    async fn refresh_bootdata(&'static self) -> bridge::Result<()> {
        self.bridge.refresh_boot_data().await?;

        // if we are now under maintenance, disconnect everyone who's still connected
        if self.bridge.is_maintenance() {
            let threads: Vec<_> = self.threads.lock().values().cloned().collect();
            for thread in threads {
                thread
                    .push_new_message(ServerThreadMessage::TerminationNotice(FastString::from_str(
                        "The server is now under maintenance, please try connecting again later",
                    )))
                    .await;
            }
        }

        Ok(())
    }
}
