use std::{
    net::{SocketAddr, SocketAddrV4},
    sync::{atomic::Ordering, Arc},
    time::Duration,
};

use globed_shared::{
    anyhow::{self, anyhow, bail},
    crypto_box::{aead::OsRng, PublicKey, SecretKey},
    esp::ByteBufferExtWrite as _,
    logger::*,
    GameServerBootData, SyncMutex, TokenIssuer, SERVER_MAGIC_LEN,
};
use rustc_hash::FxHashMap;

#[allow(unused_imports)]
use tokio::sync::oneshot; // no way

use tokio::net::UdpSocket;

use crate::{
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage, SMALL_PACKET_LIMIT},
    state::ServerState,
    util::SimpleRateLimiter,
};

const MAX_PACKET_SIZE: usize = 8192;

pub struct GameServerConfiguration {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
}

pub struct GameServer {
    pub state: ServerState,
    pub socket: UdpSocket,
    pub threads: SyncMutex<FxHashMap<SocketAddrV4, Arc<GameServerThread>>>,
    rate_limiters: SyncMutex<FxHashMap<SocketAddrV4, SimpleRateLimiter>>,
    pub secret_key: SecretKey,
    pub public_key: PublicKey,
    pub central_conf: SyncMutex<GameServerBootData>,
    pub config: GameServerConfiguration,
    pub standalone: bool,
    pub token_issuer: TokenIssuer,
}

impl GameServer {
    pub fn new(
        socket: UdpSocket,
        state: ServerState,
        central_conf: GameServerBootData,
        config: GameServerConfiguration,
        standalone: bool,
    ) -> Self {
        let secret_key = SecretKey::generate(&mut OsRng);
        let public_key = secret_key.public_key();
        let token_issuer = TokenIssuer::new(&central_conf.secret_key2, Duration::from_secs(central_conf.token_expiry));

        Self {
            state,
            socket,
            threads: SyncMutex::new(FxHashMap::default()),
            rate_limiters: SyncMutex::new(FxHashMap::default()),
            secret_key,
            public_key,
            central_conf: SyncMutex::new(central_conf),
            config,
            standalone,
            token_issuer,
        }
    }

    pub async fn run(&'static self) -> ! {
        info!("Server launched on {}", self.socket.local_addr().unwrap());

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

        // spawn stale rate limiter remover (runs once an hour)
        tokio::spawn(async {
            let mut interval = tokio::time::interval(Duration::from_secs(3600));
            interval.tick().await;

            loop {
                interval.tick().await;
                self.remove_stale_rate_limiters();
            }
        });

        // print some useful stats every once in a bit
        let interval = self.central_conf.lock().status_print_interval;

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

        // preallocate a buffer
        let mut buf = [0u8; MAX_PACKET_SIZE];

        loop {
            match self.recv_and_handle(&mut buf).await {
                Ok(()) => {}
                Err(err) => {
                    warn!("Failed to handle a packet: {err}");
                }
            }
        }
    }

    /* various calls for other threads */

    pub fn broadcast_voice_packet(
        &'static self,
        vpkt: &Arc<VoiceBroadcastPacket>,
        level_id: i32,
        room_id: u32,
    ) -> anyhow::Result<()> {
        self.broadcast_user_message(
            &ServerThreadMessage::BroadcastVoice(vpkt.clone()),
            vpkt.player_id,
            level_id,
            room_id,
        )
    }

    pub fn broadcast_chat_packet(
        &'static self,
        tpkt: &ChatMessageBroadcastPacket,
        level_id: i32,
        room_id: u32,
    ) -> anyhow::Result<()> {
        self.broadcast_user_message(
            &ServerThreadMessage::BroadcastText(tpkt.clone()),
            tpkt.player_id,
            level_id,
            room_id,
        )
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
            .filter(|thr| thr.room_id.load(Ordering::Relaxed) == room_id)
            .map(|thread| {
                thread
                    .account_data
                    .lock()
                    .make_room_preview(thread.level_id.load(Ordering::Relaxed))
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

    pub fn chat_blocked(&'static self, user_id: i32) -> bool {
        self.central_conf.lock().no_chat.contains(&user_id)
    }

    /// If someone is already logged in under the given account ID, logs them out.
    pub fn check_already_logged_in(&'static self, user_id: i32) -> anyhow::Result<()> {
        let thread = self
            .threads
            .lock()
            .values()
            .find(|thr| thr.account_id.load(Ordering::Relaxed) == user_id)
            .cloned();

        if let Some(thread) = thread {
            thread.push_new_message(ServerThreadMessage::TerminationNotice(FastString::from_str(
                "Someone logged into the same account from a different place.",
            )))?;
        }

        Ok(())
    }

    /* private handling stuff */

    /// broadcast a message to all people on the level
    fn broadcast_user_message(
        &'static self,
        msg: &ServerThreadMessage,
        origin_id: i32,
        level_id: i32,
        room_id: u32,
    ) -> anyhow::Result<()> {
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
            thread.push_new_message(msg.clone())?;
        }

        Ok(())
    }

    async fn recv_and_handle(&'static self, buf: &mut [u8]) -> anyhow::Result<()> {
        let (len, peer) = self.socket.recv_from(buf).await?;

        let peer = match peer {
            SocketAddr::V4(x) => x,
            SocketAddr::V6(_) => bail!("rejecting request from ipv6 host"),
        };

        // block packets if the client is sending too many of them
        if self.is_rate_limited(peer) {
            if cfg!(debug_assertions) {
                bail!("{peer} is ratelimited");
            }

            // silently drop the packet in release mode
            return Ok(());
        }

        // if it's a small packet like ping, we don't need to send it via the channel and can handle it right here
        if self.try_fast_handle(&buf[..len], peer).await? {
            return Ok(());
        }

        let thread = self.threads.lock().get(&peer).cloned();

        let thread = if let Some(thread) = thread {
            thread
        } else {
            let thread = Arc::new(GameServerThread::new(peer, self));
            let thread_cl = thread.clone();

            tokio::spawn(async move {
                // `thread.run()` will return in either one of those 3 conditions:
                // 1. no messages sent by the peer for 60 seconds
                // 2. the channel was closed (normally impossible for that to happen)
                // 3. `thread.terminate()` was called on that thread (due to a disconnect from either side)
                // additionally, if it panics then the state of the player will be frozen forever,
                // they won't be removed from levels or the player count and that person has to restart the game to connect again.
                // so try to avoid panics please..
                thread.run().await;
                trace!("removing client: {}", peer);
                self.post_disconnect_cleanup(&thread, peer);
            });

            self.threads.lock().insert(peer, thread_cl.clone());

            thread_cl
        };

        // don't heap allocate for small packets
        let message = if len <= SMALL_PACKET_LIMIT {
            let mut smallbuf = [0u8; SMALL_PACKET_LIMIT];
            smallbuf[..len].copy_from_slice(&buf[..len]);

            ServerThreadMessage::SmallPacket((smallbuf, len as u16))
        } else {
            ServerThreadMessage::Packet(buf[..len].to_vec())
        };

        thread.push_new_message(message)?;

        Ok(())
    }

    /// Try to fast handle a packet if the packet does not require spawning a new "thread".
    /// Returns true if packet was successfully handled, in which case the data should be discarded.
    async fn try_fast_handle(&'static self, data: &[u8], peer: SocketAddrV4) -> anyhow::Result<bool> {
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

                match self.socket.try_send_to(send_bytes, peer.into()) {
                    Ok(_) => Ok(true),
                    Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                        self.socket.send_to(send_bytes, peer).await?;
                        Ok(true)
                    }
                    Err(e) => Err(e.into()),
                }
            }

            _ => Ok(false),
        }
    }

    fn is_rate_limited(&'static self, addr: SocketAddrV4) -> bool {
        let mut limiters = self.rate_limiters.lock();
        if let Some(limiter) = limiters.get_mut(&addr) {
            !limiter.try_tick()
        } else {
            let rl_request_limit = (self.central_conf.lock().tps + 5) as usize;
            let rate_limiter = SimpleRateLimiter::new(rl_request_limit, Duration::from_millis(950));
            limiters.insert(addr, rate_limiter);
            false
        }
    }

    fn post_disconnect_cleanup(&'static self, thread: &GameServerThread, peer: SocketAddrV4) {
        self.threads.lock().remove(&peer);

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

    /// Removes rate limiters of IP addresses that haven't sent a packet in a long time (10 minutes)
    fn remove_stale_rate_limiters(&'static self) {
        let mut limiters = self.rate_limiters.lock();
        limiters.retain(|_, limiter| limiter.since_last_refill() < Duration::from_secs(600));
    }

    fn print_server_status(&'static self) {
        info!("Current server stats (printed once an hour)");
        info!(
            "Player threads: {}, player count: {}",
            self.threads.lock().len(),
            self.state.player_count.load(Ordering::Relaxed)
        );
        info!("Amount of rooms: {}", self.state.room_manager.get_rooms().len());
        info!(
            "People in the global room: {}",
            self.state.room_manager.get_global().get_total_player_count()
        );
        info!("-------------------------------------------");
    }

    async fn refresh_bootdata(&'static self) -> anyhow::Result<()> {
        let response = self
            .config
            .http_client
            .post(format!("{}{}", self.config.central_url, "gs/boot"))
            .query(&[("pw", self.config.central_pw.clone())])
            .send()
            .await?
            .error_for_status()
            .map_err(|e| anyhow!("central server returned an error: {e}"))?;

        let configuration = response.bytes().await?;

        let mut reader = ByteReader::from_bytes(&configuration);
        reader.skip(SERVER_MAGIC_LEN);

        let boot_data: GameServerBootData = reader
            .read_value()
            .map_err(|e| anyhow!("central server sent malformed response: {e}"))?;

        let mut conf = self.central_conf.lock();

        let is_now_under_maintenance = !conf.maintenance && boot_data.maintenance;

        *conf = boot_data;

        // if we are now under maintenance, disconnect everyone who's still connected
        if is_now_under_maintenance {
            let threads: Vec<_> = self.threads.lock().values().cloned().collect();
            for thread in threads {
                thread.push_new_message(ServerThreadMessage::TerminationNotice(FastString::from_str(
                    "The server is now under maintenance, please try connecting again later",
                )))?;
            }
        }

        Ok(())
    }
}
