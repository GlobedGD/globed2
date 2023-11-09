use std::{net::SocketAddr, sync::Arc};

use crypto_box::{aead::OsRng, SecretKey};
use rustc_hash::FxHashMap;
#[allow(unused_imports)]
use tokio::sync::oneshot; // no way

use log::{debug, info, warn};
use tokio::{net::UdpSocket, sync::RwLock};

use crate::{server_thread::GameServerThread, state::ServerState};

pub struct GameServer {
    pub address: String,
    pub state: ServerState,
    pub socket: Arc<UdpSocket>,
    pub threads: RwLock<FxHashMap<SocketAddr, Arc<GameServerThread>>>,
    pub secret_key: SecretKey,
}

impl GameServer {
    pub async fn new(address: String, state: ServerState) -> Self {
        let secret_key = SecretKey::generate(&mut OsRng);

        Self {
            address: address.clone(),
            state,
            socket: Arc::new(UdpSocket::bind(&address).await.unwrap()),
            threads: RwLock::new(FxHashMap::default()),
            secret_key,
        }
    }

    pub async fn run(&'static self) -> anyhow::Result<()> {
        let mut buf = [0u8; 65536];

        info!("Server launched on {}", self.address);

        loop {
            match self.recv_and_handle(&mut buf).await {
                Ok(_) => {}
                Err(err) => {
                    warn!("Failed to handle a packet: {}", err.to_string());
                }
            }
        }
    }

    async fn recv_and_handle(&'static self, buf: &mut [u8]) -> anyhow::Result<()> {
        let (len, peer) = self.socket.recv_from(buf).await?;
        let threads = self.threads.read().await;
        let has_thread = threads.contains_key(&peer);

        if has_thread {
            threads
                .get(&peer)
                .unwrap()
                .send_packet(buf[..len].to_vec())
                .await?;
        } else {
            drop(threads);
            let mut threads = self.threads.write().await;

            debug!("creating new thread for {}", peer);

            let thread = Arc::new(GameServerThread::new(
                peer,
                self.socket.clone(),
                self.secret_key.clone(),
            ));
            let thread_cl = thread.clone();
            tokio::spawn(async move {
                match thread.run().await {
                    Ok(_) => {}
                    Err(err) => {
                        warn!("Client thread died: {}", err.to_string());
                    }
                };
            });

            thread_cl.send_packet(buf[..len].to_vec()).await?;

            threads.insert(peer, thread_cl);
        }

        Ok(())
    }
}
