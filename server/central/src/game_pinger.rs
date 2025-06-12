// oh lord

use std::{
    net::SocketAddr,
    sync::atomic::{AtomicU32, Ordering},
    time::{Duration, SystemTime},
};

use globed_shared::{
    SyncMutex, debug,
    esp::{ByteBuffer, ByteBufferExtRead, ByteBufferExtWrite, ByteReader},
    rand::{self, Rng},
    warn,
};
use tokio::net::UdpSocket;

use crate::config::GameServerEntry;

pub struct GameServerPinger {
    addresses: Vec<SocketAddr>,
    udp_socket_v4: UdpSocket,
    udp_socket_v6: UdpSocket,
    latest_player_count: AtomicU32,
    history: SyncMutex<Vec<(SystemTime, u32)>>,
}

impl GameServerPinger {
    pub async fn new(servers: &[GameServerEntry]) -> Self {
        let mut addresses = Vec::new();

        for server in servers {
            let addr = tokio::net::lookup_host(&server.address)
                .await
                .expect("failed to resolve provided game server address")
                .next()
                .expect("no addresses found for provided game server address");

            addresses.push(addr);
        }

        let sock_v4 = UdpSocket::bind("0.0.0.0:0").await.expect("failed to bind udp socket (v4) for pinger");
        let sock_v6 = UdpSocket::bind("[::]:0").await.expect("failed to bind udp socket (v6) for pinger");

        Self {
            addresses,
            udp_socket_v4: sock_v4,
            udp_socket_v6: sock_v6,
            latest_player_count: AtomicU32::new(0),
            history: SyncMutex::new(Vec::new()),
        }
    }

    pub fn get_player_count(&self) -> u32 {
        self.latest_player_count.load(Ordering::SeqCst)
    }

    pub fn get_player_count_history(&self) -> Vec<(SystemTime, u32)> {
        let mut history = self.history.lock();
        std::mem::take(&mut *history)
    }

    pub async fn run_pinger(&self) -> ! {
        // fetch every 3 minutes
        let mut interval = tokio::time::interval(Duration::from_secs(180));

        // ping packet LOL
        let mut buffer = ByteBuffer::new();
        let ping_id = rand::rng().random();
        buffer.write_u16(10000);
        buffer.write_bool(false);
        buffer.write_u32(ping_id);

        loop {
            interval.tick().await;

            // ping all active game servers
            debug!("pinging {} servers", self.addresses.len());

            for address in &self.addresses {
                if address.is_ipv4() {
                    let _ = self.udp_socket_v4.send_to(buffer.as_bytes(), address).await;
                } else if address.is_ipv6() {
                    let _ = self.udp_socket_v6.send_to(buffer.as_bytes(), address).await;
                }
            }

            let player_count = self.receive_responses(ping_id, self.addresses.len()).await;

            debug!("total player count: {player_count}");

            self.latest_player_count.store(player_count, Ordering::SeqCst);
            let mut history = self.history.lock();
            history.push((SystemTime::now(), player_count));
        }
    }

    async fn receive_responses(&self, ping_id: u32, max: usize) -> u32 {
        let mut total_players = 0;
        let mut successful_requests = 0;

        let mut buf = [0u8; 512];
        let mut buf2 = [0u8; 512];

        while successful_requests < max {
            match tokio::time::timeout(Duration::from_secs(5), self.receive_any(&mut buf, &mut buf2)).await {
                Ok((Ok(_), ipv6)) => {
                    let mut buffer = ByteReader::from_bytes(if ipv6 { &buf2 } else { &buf });
                    // skip header
                    buffer.skip(3);
                    let s_ping_id = buffer.read_u32().unwrap_or(0);
                    let s_player_count = buffer.read_u32().unwrap_or(0);

                    if s_ping_id != ping_id {
                        continue;
                    }

                    total_players += s_player_count;
                    successful_requests += 1;
                }
                Ok((Err(e), _)) => {
                    warn!("failed to recv data: {e}");
                    break;
                }
                Err(_) => break,
            }
        }

        total_players
    }

    async fn receive_any(&self, buf: &mut [u8], buf2: &mut [u8]) -> (std::io::Result<usize>, bool) {
        tokio::select! {
            res = self.udp_socket_v4.recv(buf) => (res, false),
            res = self.udp_socket_v6.recv(buf2) => (res, true),
        }
    }
}
