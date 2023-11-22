use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, Ordering},
        Arc,
    },
    time::{Duration, SystemTime},
};

use parking_lot::Mutex as SyncMutex;

use anyhow::{anyhow, bail};
use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, OsRng},
    ChaChaBox,
};
use globed_shared::PROTOCOL_VERSION;
use log::{debug, warn};
use tokio::sync::{
    mpsc::{self, Receiver, Sender},
    Mutex,
};

use crate::{
    bytebufferext::{ByteBufferExt, ByteBufferExtRead, ByteBufferExtWrite, Decodable},
    data::{
        packets::{client::*, server::*, Packet, PacketWithId, PACKET_HEADER_LEN},
        types::{AssociatedPlayerData, CryptoPublicKey, PlayerAccountData},
    },
    server::GameServer,
};

pub const SMALL_PACKET_LIMIT: usize = 128;
const CHANNEL_BUFFER: usize = 4;

pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    SmallPacket([u8; SMALL_PACKET_LIMIT]),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
}

pub struct GameServerThread {
    game_server: &'static GameServer,

    rx: Mutex<Receiver<ServerThreadMessage>>,
    tx: Sender<ServerThreadMessage>,
    awaiting_termination: AtomicBool,
    pub authenticated: AtomicBool,
    crypto_box: SyncMutex<Option<ChaChaBox>>,

    peer: SocketAddrV4,
    pub account_id: AtomicI32,
    pub level_id: AtomicI32,
    pub account_data: SyncMutex<PlayerAccountData>,

    last_voice_packet: SyncMutex<SystemTime>,
}

macro_rules! gs_require {
    ($cond:expr,$msg:literal) => {
        if !($cond) {
            bail!($msg);
        }
    };
}

macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        async fn $name(&$self, buf: &mut ByteReader<'_>) -> anyhow::Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            $code
        }
    };
}

macro_rules! gs_disconnect {
    ($self:ident, $msg:expr) => {
        $self.terminate();
        $self.send_packet(&ServerDisconnectPacket { message: $msg }).await?;
        return Ok(());
    };
}

#[allow(unused_macros)]
macro_rules! gs_notice {
    ($self:expr, $msg:expr) => {
        $self.send_packet(&ServerNoticePacket { message: $msg }).await?;
    };
}

macro_rules! gs_needauth {
    ($self:ident) => {
        if !$self.authenticated.load(Ordering::Relaxed) {
            gs_disconnect!($self, "unauthorized".to_string());
        }
    };
}

impl GameServerThread {
    /* public api for the main server */

    pub fn new(peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        let (tx, rx) = mpsc::channel::<ServerThreadMessage>(CHANNEL_BUFFER);
        Self {
            tx,
            rx: Mutex::new(rx),
            peer,
            crypto_box: SyncMutex::new(None),
            account_id: AtomicI32::new(0),
            level_id: AtomicI32::new(0),
            authenticated: AtomicBool::new(false),
            game_server,
            awaiting_termination: AtomicBool::new(false),
            account_data: SyncMutex::new(PlayerAccountData::default()),
            last_voice_packet: SyncMutex::new(SystemTime::now()),
        }
    }

    pub async fn run(&self) -> anyhow::Result<()> {
        let mut rx = self.rx.lock().await;

        loop {
            if self.awaiting_termination.load(Ordering::Relaxed) {
                break;
            }

            match tokio::time::timeout(Duration::from_secs(60), rx.recv()).await {
                Ok(Some(message)) => match self.handle_message(message).await {
                    Ok(_) => {}
                    Err(err) => warn!("[@{}]: {}", self.peer, err.to_string()),
                },
                Ok(None) => break, // sender closed
                Err(_) => break,   // timeout
            };
        }

        Ok(())
    }

    pub async fn send_message(&self, data: ServerThreadMessage) -> anyhow::Result<()> {
        self.tx.send(data).await?;
        Ok(())
    }

    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    /* private utilities */

    async fn send_packet(&self, packet: &impl Packet) -> anyhow::Result<()> {
        let serialized = self.serialize_packet(packet)?;
        self.send_buffer(serialized.as_bytes()).await
    }

    async fn send_buffer(&self, buffer: &[u8]) -> anyhow::Result<()> {
        self.game_server.socket.send_to(buffer, self.peer).await?;
        Ok(())
    }

    fn serialize_packet(&self, packet: &impl Packet) -> anyhow::Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_u16(packet.get_packet_id());
        buf.write_bool(packet.get_encrypted());

        if !packet.get_encrypted() {
            packet.encode(&mut buf);
            return Ok(buf);
        }

        let cbox = self.crypto_box.lock();

        #[cfg(debug_assertions)]
        gs_require!(
            cbox.is_some(),
            "trying to send an encrypted packet when no cryptobox was initialized"
        );

        let mut cltxtbuf = ByteBuffer::new();
        packet.encode(&mut cltxtbuf);

        let cbox = cbox.as_ref().unwrap();
        let nonce = ChaChaBox::generate_nonce(&mut OsRng);

        let encrypted = cbox.encrypt(&nonce, cltxtbuf.as_bytes())?;

        #[cfg(not(rust_analyzer))] // i am so sorry
        buf.write_bytes(&nonce);
        buf.write_bytes(&encrypted);

        Ok(buf)
    }

    async fn handle_message(&self, message: ServerThreadMessage) -> anyhow::Result<()> {
        match message {
            ServerThreadMessage::Packet(data) => match self.handle_packet(&data).await {
                Ok(_) => {}
                Err(err) => bail!("failed to handle packet: {err}"),
            },

            ServerThreadMessage::SmallPacket(data) => match self.handle_packet(&data).await {
                Ok(_) => {}
                Err(err) => bail!("failed to handle packet: {err}"),
            },

            ServerThreadMessage::BroadcastVoice(voice_packet) => match self.send_packet(&*voice_packet).await {
                Ok(_) => {}
                Err(err) => bail!("failed to broadcast voice packet: {err}"),
            },
        }

        Ok(())
    }

    /* packet handlers */

    async fn handle_packet(&self, message: &[u8]) -> anyhow::Result<()> {
        #[cfg(debug_assertions)]
        gs_require!(message.len() >= PACKET_HEADER_LEN, "packet is missing a header");

        let mut data = ByteReader::from_bytes(message);

        let packet_id = data.read_u16()?;
        let encrypted = data.read_bool()?;

        // for optimization, reject the voice packet immediately if the player is blocked from vc
        if packet_id == VoicePacket::PACKET_ID {
            let accid = self.account_id.load(Ordering::Relaxed);
            if self.game_server.chat_blocked(accid) {
                debug!("blocking voice packet from {accid}");
                return Ok(());
            }
        }

        let cleartext_vec;
        if encrypted {
            let cbox = self.crypto_box.lock();

            gs_require!(
                cbox.is_some(),
                "attempting to decode an encrypted packet when no cryptobox was initialized"
            );

            let encrypted_data = data.read_bytes(data.len() - data.get_rpos())?;
            let nonce = &encrypted_data[..24];
            let rest = &encrypted_data[24..];

            let cbox = cbox.as_ref().unwrap();
            cleartext_vec = cbox.decrypt(nonce.into(), rest)?;

            data = ByteReader::from_bytes(&cleartext_vec);
        }

        // minor optimization
        if packet_id == PlayerDataPacket::PACKET_ID {
            return self.handle_player_data(&mut data).await;
        }

        if packet_id == LoginPacket::PACKET_ID && !encrypted {
            bail!("trying to login with cleartext credentials");
        }

        match packet_id {
            /* connection related */
            PingPacket::PACKET_ID => self.handle_ping(&mut data).await,
            CryptoHandshakeStartPacket::PACKET_ID => self.handle_crypto_handshake(&mut data).await,
            KeepalivePacket::PACKET_ID => self.handle_keepalive(&mut data).await,
            LoginPacket::PACKET_ID => self.handle_login(&mut data).await,
            DisconnectPacket::PACKET_ID => self.handle_disconnect(&mut data).await,

            /* game related */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            x => Err(anyhow!("No handler for packet id {x}")),
        }
    }

    gs_handler!(self, handle_ping, PingPacket, packet, {
        self.send_packet(&PingResponsePacket {
            id: packet.id,
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await?;

        Ok(())
    });

    gs_handler!(self, handle_crypto_handshake, CryptoHandshakeStartPacket, packet, {
        match packet.protocol {
            p if p > PROTOCOL_VERSION => {
                gs_disconnect!(
                    self,
                    format!(
                        "Outdated server! You are running protocol v{p} while the server is still on v{PROTOCOL_VERSION}.",
                    )
                );
            }
            p if p < PROTOCOL_VERSION => {
                gs_disconnect!(
                    self,
                    format!(
                        "Outdated client! Please update the mod in order to connect to the server. Client protocol version: v{p}, server: v{PROTOCOL_VERSION}",
                    )
                );
            }
            _ => {}
        }

        {
            let mut cbox = self.crypto_box.lock();

            // as ServerThread is now tied to the SocketAddrV4 and not account id like in globed v0
            // erroring here is not a concern, even if the user's game crashes without a disconnect packet,
            // they would have a new randomized port when they restart and this would never fail.
            gs_require!(cbox.is_none(), "attempting to initialize a cryptobox twice");

            let sbox = ChaChaBox::new(&packet.key.pubkey, &self.game_server.secret_key);
            *cbox = Some(sbox);
        }

        self.send_packet(&CryptoHandshakeResponsePacket {
            key: CryptoPublicKey {
                pubkey: self.game_server.secret_key.public_key().clone(),
            },
        })
        .await?;

        Ok(())
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        gs_needauth!(self);

        self.send_packet(&KeepaliveResponsePacket {
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await?;

        Ok(())
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        // lets verify the given token
        let url = format!("{}gs/verify", self.game_server.config.central_url);

        let response = self
            .game_server
            .config
            .http_client
            .post(url)
            .query(&[
                ("account_id", packet.account_id.to_string()),
                ("token", packet.token.clone()),
                ("pw", self.game_server.config.central_pw.clone()),
            ])
            .send()
            .await?
            .error_for_status()?
            .text()
            .await?;

        if !response.starts_with("status_ok:") {
            self.terminate();
            self.send_packet(&LoginFailedPacket {
                message: format!("authentication failed: {}", response),
            })
            .await?;

            return Ok(());
        }

        let player_name = response.split_once(':').ok_or(anyhow!("central server is drunk"))?.1;

        self.authenticated.store(true, Ordering::Relaxed);
        self.account_id.store(packet.account_id, Ordering::Relaxed);
        self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed); // increment player count

        {
            let mut account_data = self.account_data.lock();
            account_data.account_id = packet.account_id;
            account_data.name = player_name.to_string();

            let special_user_data = self
                .game_server
                .central_conf
                .lock()
                .special_users
                .get(&packet.account_id)
                .cloned();

            if let Some(sud) = special_user_data {
                account_data.special_user_data = Some(sud.try_into()?);
            }
        }

        debug!("Login successful from {player_name} ({})", packet.account_id);

        self.send_packet(&LoggedInPacket {}).await?;

        Ok(())
    });

    gs_handler!(self, handle_disconnect, DisconnectPacket, _packet, {
        self.terminate();
        Ok(())
    });

    /* game related */
    gs_handler!(self, handle_sync_icons, SyncIconsPacket, packet, {
        gs_needauth!(self);

        let mut account_data = self.account_data.lock();
        account_data.icons.clone_from(&packet.icons);
        Ok(())
    });

    gs_handler!(self, handle_request_profiles, RequestProfilesPacket, packet, {
        gs_needauth!(self);

        self.send_packet(&PlayerProfilesPacket {
            profiles: self.game_server.gather_profiles(&packet.ids).await,
        })
        .await?;

        Ok(())
    });

    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        gs_needauth!(self);

        let account_id = self.account_id.load(Ordering::Relaxed);
        let old_level = self.level_id.swap(packet.level_id, Ordering::Relaxed);

        let mut pm = self.game_server.state.player_manager.lock();

        if old_level != 0 {
            pm.remove_from_level(old_level, account_id)
        }

        pm.add_to_level(packet.level_id, account_id);

        Ok(())
    });

    gs_handler!(self, handle_level_leave, LevelLeavePacket, _packet, {
        gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id != 0 {
            let account_id = self.account_id.load(Ordering::Relaxed);

            let mut pm = self.game_server.state.player_manager.lock();
            pm.remove_from_level(level_id, account_id);
        }

        Ok(())
    });

    // if you are seeing this. i am so sorry.
    gs_handler!(self, handle_player_data, PlayerDataPacket, packet, {
        gs_needauth!(self);

        let level_id = self.level_id.load(Ordering::Relaxed);
        if level_id == 0 {
            bail!("player sending PlayerDataPacket when not on a level");
        }

        let account_id = self.account_id.load(Ordering::Relaxed);

        let mut buf: ByteBuffer;

        {
            let mut pm = self.game_server.state.player_manager.lock();
            pm.set_player_data(account_id, &packet.data);

            // this unwrap should be safe
            let players = pm.get_players_on_level(level_id).unwrap();

            let calc_size = PACKET_HEADER_LEN + 4 + ((players.len() - 1) * AssociatedPlayerData::encoded_size());
            debug!("alloc with capacity: {calc_size}");
            buf = ByteBuffer::with_capacity(calc_size);

            // dont actually do this anywhere else please
            buf.write_u16(LevelDataPacket::PACKET_ID);
            buf.write_bool(false);
            buf.write_u32(players.len() as u32 - 1); // minus ourselves

            for player in players.iter() {
                if player.account_id == account_id {
                    continue;
                }

                buf.write_value(*player);
            }
            debug!("size at the end: {}", buf.len());
        }

        self.send_buffer(buf.as_bytes()).await?;

        Ok(())
    });

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        gs_needauth!(self);

        let accid = self.account_id.load(Ordering::Relaxed);

        // check the throughput
        {
            let mut last_voice_packet = self.last_voice_packet.lock();
            let now = SystemTime::now();
            let passed_time = now.duration_since(*last_voice_packet)?.as_millis();
            *last_voice_packet = now;

            let total_size = packet.data.opus_frames.iter().map(|frame| frame.len()).sum::<usize>();

            let throughput = (total_size as f32) / (passed_time as f32); // in kb/s

            debug!("voice packet throughput: {}kb/s", throughput);
            if throughput > 8f32 {
                warn!("rejecting a voice packet, throughput above the limit: {}kb/s", throughput);
                return Ok(());
            }
        }

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: accid,
            data: packet.data.clone(),
        });

        self.game_server.broadcast_voice_packet(vpkt).await?;

        Ok(())
    });
}
