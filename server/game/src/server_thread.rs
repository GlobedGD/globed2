use std::{
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, Ordering},
        Arc,
    },
    time::Duration,
};

use anyhow::anyhow;
use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, OsRng},
    SalsaBox, SecretKey,
};
use log::warn;
use tokio::{
    net::UdpSocket,
    sync::{
        mpsc::{self, Receiver, Sender},
        Mutex,
    },
};

use crate::{
    data::{
        packets::{client::*, match_packet, server::*, Packet, PacketWithId, PACKET_HEADER_LEN},
        types::crypto::CryptoPublicKey,
    },
    server::GameServer,
    state::ServerState,
};

pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    BroadcastVoice(VoiceBroadcastPacket),
}

pub struct GameServerThread {
    state: ServerState,
    rx: Mutex<Receiver<ServerThreadMessage>>,
    tx: Sender<ServerThreadMessage>,
    peer: SocketAddrV4,
    socket: Arc<UdpSocket>,
    secret_key: SecretKey,
    crypto_box: Mutex<Option<SalsaBox>>,
    account_id: AtomicI32,
    authenticated: AtomicBool,
    game_server: &'static GameServer,
    awaiting_termination: AtomicBool,
}

macro_rules! gs_assert {
    ($cond:expr,$msg:literal) => {
        if !($cond) {
            return Err(anyhow!($msg));
        }
    };
}

macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        // Insanity if you ask me
        async fn $name(&$self, packet: Box<dyn Packet>) -> anyhow::Result<Option<Box<dyn Packet>>> {
            let _tmp = packet.as_any().downcast_ref::<$pktty>();
            if _tmp.is_none() {
                return Err(anyhow!("failed to downcast packet"));
            }
            let $pkt = _tmp.unwrap();
            $code
        }
    };
}

macro_rules! gs_retpacket {
    ($code:expr) => {
        return Ok(Some(Box::new($code)))
    };
}

macro_rules! gs_disconnect {
    ($self:ident,$msg:expr) => {
        $self.terminate();
        return Ok(Some(Box::new(ServerDisconnectPacket { message: $msg })))
    };
}

macro_rules! gs_needauth {
    ($self:ident) => {
        if !$self.authenticated.load(Ordering::Relaxed) {
            return Ok(Some(Box::new(ServerDisconnectPacket {
                message: "not logged in".to_string(),
            })));
        }
    };
}

impl GameServerThread {
    /* public api for the main server */

    pub fn new(
        state: ServerState,
        peer: SocketAddrV4,
        socket: Arc<UdpSocket>,
        secret_key: SecretKey,
        game_server: &'static GameServer,
    ) -> Self {
        let (tx, rx) = mpsc::channel::<ServerThreadMessage>(8);
        Self {
            state,
            tx,
            rx: Mutex::new(rx),
            peer,
            secret_key,
            socket,
            crypto_box: Mutex::new(None),
            account_id: AtomicI32::new(0),
            authenticated: AtomicBool::new(false),
            game_server,
            awaiting_termination: AtomicBool::new(false),
            // last_keepalive: Mutex::new(SystemTime::now()),
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
                    Err(err) => warn!("{}", err.to_string()),
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

    async fn send_packet(&self, packet: Box<dyn Packet>) -> anyhow::Result<()> {
        let serialized = self.serialize_packet(packet).await?;
        self.socket.send_to(serialized.as_bytes(), self.peer).await?;

        Ok(())
    }

    async fn parse_packet(&self, message: Vec<u8>) -> anyhow::Result<Box<dyn Packet>> {
        gs_assert!(message.len() > PACKET_HEADER_LEN, "packet is missing a header");

        let mut data = ByteReader::from_bytes(&message);

        let packet_id = data.read_u16()?;
        let encrypted = data.read_u8()? != 0u8;

        let packet = match_packet(packet_id);
        gs_assert!(packet.is_some(), "invalid packet was sent");

        let mut packet = packet.unwrap();
        if packet.get_encrypted() && !encrypted {
            gs_assert!(false, "client sent a cleartext packet when expected an encrypted one");
        }

        if !packet.get_encrypted() {
            packet.decode_from_reader(&mut data)?;
            return Ok(packet);
        }

        let cbox = self.crypto_box.lock().await;

        gs_assert!(
            cbox.is_some(),
            "attempting to decode an encrypted packet when no cryptobox was initialized"
        );

        let encrypted_data = data.read_bytes(data.len() - data.get_rpos())?;
        let nonce = &encrypted_data[..24];
        let rest = &encrypted_data[24..];

        let cbox = cbox.as_ref().unwrap();
        let cleartext = cbox.decrypt(nonce.into(), rest)?;

        let mut packetbuf = ByteReader::from_bytes(&cleartext);
        packet.decode_from_reader(&mut packetbuf)?;

        Ok(packet)
    }

    async fn serialize_packet(&self, packet: Box<dyn Packet>) -> anyhow::Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_u16(packet.get_packet_id());
        buf.write_u8(if packet.get_encrypted() { 1u8 } else { 0u8 });

        if !packet.get_encrypted() {
            packet.encode(&mut buf);
            return Ok(buf);
        }

        let cbox = self.crypto_box.lock().await;

        gs_assert!(
            cbox.is_some(),
            "trying to send an encrypted packet when no cryptobox was initialized"
        );

        let mut cltxtbuf = ByteBuffer::new();
        packet.encode(&mut cltxtbuf);

        let cbox = cbox.as_ref().unwrap();
        let nonce = SalsaBox::generate_nonce(&mut OsRng);

        let encrypted = cbox.encrypt(&nonce, cltxtbuf.as_bytes())?;

        buf.write_bytes(&nonce);
        buf.write_bytes(&encrypted);

        Ok(buf)
    }

    async fn handle_message(&self, message: ServerThreadMessage) -> anyhow::Result<()> {
        match message {
            ServerThreadMessage::Packet(message) => match self.parse_packet(message).await {
                Ok(packet) => match self.handle_packet(packet).await {
                    Ok(_) => {}
                    Err(err) => return Err(anyhow!("failed to handle packet: {}", err.to_string())),
                },
                Err(err) => return Err(anyhow!("failed to parse packet: {}", err.to_string())),
            },

            ServerThreadMessage::BroadcastVoice(voice_packet) => match self.send_packet(Box::new(voice_packet)).await {
                Ok(_) => {}
                Err(err) => {
                    warn!("failed to broadcast voice packet: {}", err.to_string())
                }
            },
        }

        Ok(())
    }

    /* packet handlers */

    async fn handle_packet(&self, packet: Box<dyn Packet>) -> anyhow::Result<()> {
        let response = match packet.get_packet_id() {
            /* connection related */
            PingPacket::PACKET_ID => self.handle_ping(packet).await,
            CryptoHandshakeStartPacket::PACKET_ID => self.handle_crypto_handshake(packet).await,
            KeepalivePacket::PACKET_ID => self.handle_keepalive(packet).await,
            LoginPacket::PACKET_ID => self.handle_login(packet).await,
            DisconnectPacket::PACKET_ID => self.handle_disconnect(packet).await,

            /* level related */
            VoicePacket::PACKET_ID => self.handle_voice(packet).await,
            x => Err(anyhow!("No handler for packet id {x}")),
        }?;

        if let Some(response_packet) = response {
            self.send_packet(response_packet).await?;
        }

        Ok(())
    }

    gs_handler!(self, handle_ping, PingPacket, packet, {
        gs_retpacket!(PingResponsePacket {
            id: packet.id,
            player_count: 2
        });
    });

    gs_handler!(self, handle_crypto_handshake, CryptoHandshakeStartPacket, packet, {
        let mut cbox = self.crypto_box.lock().await;
        gs_assert!(cbox.is_none(), "attempting to initialize a cryptobox twice");

        let sbox = SalsaBox::new(&packet.key.pubkey, &self.secret_key);
        *cbox = Some(sbox);

        gs_retpacket!(CryptoHandshakeResponsePacket {
            key: CryptoPublicKey {
                pubkey: self.secret_key.public_key().clone()
            }
        });
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        gs_retpacket!(KeepaliveResponsePacket {
            player_count: self.game_server.get_player_count().await as u32
        })
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        // lets verify the given token
        let state = self.state.read().await;
        let client = state.http_client.clone();
        let central_url = state.central_url.clone();
        let pw = state.central_pw.clone();
        drop(state);

        let url = format!("{central_url}gs/verify");
        let response = client
            .post(url)
            .query(&[
                ("account_id", packet.account_id.to_string()),
                ("token", packet.token.clone()),
                ("pw", pw),
            ])
            .send()
            .await?
            .error_for_status()?
            .text()
            .await?;

        if !response.starts_with("status_ok:") {
            gs_disconnect!(self, format!("failed to authenticate: {response}"));
        }

        let player_name = response.split_once(':').ok_or(anyhow!("central server is drunk"))?.1;

        self.authenticated.store(true, Ordering::Relaxed);
        self.account_id.store(packet.account_id, Ordering::Relaxed);

        gs_retpacket!(LoggedInPacket {})
    });

    gs_handler!(self, handle_disconnect, DisconnectPacket, _packet, {
        self.terminate();
        return Ok(None);
    });

    /* level related */

    gs_handler!(self, handle_voice, VoicePacket, packet, {
        gs_needauth!(self);

        let vpkt = VoiceBroadcastPacket {
            player_id: self.account_id.load(Ordering::Relaxed),
            data: packet.data.clone(),
        };

        self.game_server.broadcast_voice_packet(&vpkt).await?;

        Ok(None)
    });
}
