use std::{
    net::{SocketAddr, SocketAddrV4},
    sync::{atomic::Ordering, Arc},
    time::Duration,
};

use parking_lot::Mutex as SyncMutex;

use anyhow::{anyhow, bail};
use crypto_box::{aead::OsRng, SecretKey};
use globed_shared::GameServerBootData;
use rustc_hash::FxHashMap;

#[allow(unused_imports)]
use tokio::sync::oneshot; // no way

use log::{debug, error, info, warn};
use tokio::net::UdpSocket;

use crate::{
    data::*,
    server_thread::{GameServerThread, ServerThreadMessage, SMALL_PACKET_LIMIT},
    state::ServerState,
};

const MAX_PACKET_SIZE: usize = 16384;

pub struct GameServerConfiguration {
    pub http_client: reqwest::Client,
    pub central_url: String,
    pub central_pw: String,
}

pub struct GameServer {
    pub state: ServerState,
    pub socket: UdpSocket,
    pub threads: SyncMutex<FxHashMap<SocketAddrV4, Arc<GameServerThread>>>,
    pub secret_key: SecretKey,
    pub central_conf: SyncMutex<GameServerBootData>,
    pub config: GameServerConfiguration,
    pub standalone: bool,
}

impl GameServer {
    pub async fn new(
        address: String,
        state: ServerState,
        central_conf: GameServerBootData,
        config: GameServerConfiguration,
        standalone: bool,
    ) -> Self {
        Self {
            state,
            socket: UdpSocket::bind(&address).await.unwrap(),
            threads: SyncMutex::new(FxHashMap::default()),
            secret_key: SecretKey::generate(&mut OsRng),
            central_conf: SyncMutex::new(central_conf),
            config,
            standalone,
        }
    }

    pub async fn run(&'static self) -> anyhow::Result<()> {
        info!("Server launched on {}", self.socket.local_addr()?);

        if !self.standalone {
            tokio::spawn(async move {
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

        // we preallocate a buffer to avoid zeroing out MAX_PACKET_SIZE bytes on each packet
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

    pub fn broadcast_voice_packet(&'static self, vpkt: &Arc<VoiceBroadcastPacket>, level_id: i32) -> anyhow::Result<()> {
        self.broadcast_user_message(&ServerThreadMessage::BroadcastVoice(vpkt.clone()), vpkt.player_id, level_id)
    }

    pub fn broadcast_chat_packet(&'static self, tpkt: &ChatMessageBroadcastPacket, level_id: i32) -> anyhow::Result<()> {
        self.broadcast_user_message(&ServerThreadMessage::BroadcastText(tpkt.clone()), tpkt.player_id, level_id)
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
            .filter(|thr| thr.authenticated.load(Ordering::Relaxed))
            .map(|thread| thread.account_data.lock().make_preview())
            .fold(0, |count, preview| count + usize::from(f(&preview, count, additional)))
    }

    pub fn chat_blocked(&'static self, user_id: i32) -> bool {
        self.central_conf.lock().no_chat.contains(&user_id)
    }

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
    ) -> anyhow::Result<()> {
        let pm = self.state.player_manager.lock();
        let players = pm.get_level(level_id);

        if let Some(players) = players {
            let threads: Vec<_> = self
                .threads
                .lock()
                .values()
                .filter(|thread| {
                    let account_id = thread.account_id.load(Ordering::Relaxed);
                    account_id != origin_id && players.contains(&account_id)
                })
                .cloned()
                .collect();

            drop(pm);

            for thread in threads {
                thread.push_new_message(msg.clone())?;
            }
        }

        Ok(())
    }

    async fn recv_and_handle(&'static self, buf: &mut [u8]) -> anyhow::Result<()> {
        let (len, peer) = self.socket.recv_from(buf).await?;

        let peer = match peer {
            SocketAddr::V6(_) => bail!("rejecting request from ipv6 host"),
            SocketAddr::V4(x) => x,
        };

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
                log::trace!("removing client: {}", peer);
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

    fn post_disconnect_cleanup(&'static self, thread: &GameServerThread, peer: SocketAddrV4) {
        self.threads.lock().remove(&peer);

        if !thread.authenticated.load(Ordering::Relaxed) {
            return;
        }

        let account_id = thread.account_id.load(Ordering::Relaxed);
        let level_id = thread.level_id.load(Ordering::Relaxed);

        // decrement player count
        self.state.player_count.fetch_sub(1, Ordering::Relaxed);

        // remove from the player manager and the level if they are on one
        let mut pm = self.state.player_manager.lock();
        pm.remove_player(account_id);

        if level_id != 0 {
            pm.remove_from_level(level_id, account_id);
        }
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

        let configuration = response.text().await?;
        let boot_data: GameServerBootData = serde_json::from_str(&configuration)?;

        *self.central_conf.lock() = boot_data;

        Ok(())
    }
}
