use std::{net::SocketAddr, sync::Arc};

use anyhow::anyhow;
use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, OsRng},
    SalsaBox, SecretKey,
};
use log::{debug, warn};
use tokio::{
    net::UdpSocket,
    sync::{
        mpsc::{self, Receiver, Sender},
        Mutex,
    },
};

use crate::data::packets::{client::*, match_packet, server::*, Packet, PACKET_HEADER_LEN};

pub struct GameServerThread {
    rx: Mutex<Receiver<Vec<u8>>>,
    tx: Sender<Vec<u8>>,
    peer: SocketAddr,
    socket: Arc<UdpSocket>,
    secret_key: SecretKey,
    crypto_box: Mutex<Option<SalsaBox>>,
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
        return Ok(Some(Box::new($code)));
    };
}

impl GameServerThread {
    /* Packet handlers for packet types  */

    gs_handler!(self, handle_ping, PingPacket, packet, {
        gs_retpacket!(PingResponsePacket {
            id: packet.id,
            player_count: 2
        });
    });

    gs_handler!(
        self,
        handle_crypto_handshake,
        CryptoHandshakeStartPacket,
        packet,
        {
            let mut cbox = self.crypto_box.lock().await;
            gs_assert!(cbox.is_none(), "attempting to initialize a cryptobox twice");

            let sbox = SalsaBox::new(&packet.data.pubkey, &self.secret_key);
            *cbox = Some(sbox);

            Ok(None)
        }
    );

    /* All the other stuff */
    async fn handle_packet(&self, packet: Box<dyn Packet>) -> anyhow::Result<()> {
        let response = match packet.get_packet_id() {
            10000 => self.handle_ping(packet).await,
            10001 => self.handle_crypto_handshake(packet).await,
            x => Err(anyhow!("No handler for packet id {x}")),
        }?;

        if let Some(response_packet) = response {
            let serialized = self.serialize_packet(response_packet).await?;
            self.socket
                .send_to(serialized.as_bytes(), self.peer)
                .await?;
        }

        Ok(())
    }

    async fn parse_packet(&self, message: Vec<u8>) -> anyhow::Result<Box<dyn Packet>> {
        gs_assert!(
            message.len() > PACKET_HEADER_LEN,
            "packet is missing a header"
        );

        let mut data = ByteReader::from_bytes(&message);

        let packet_id = data.read_u16()?;
        let encrypted = data.read_u8()? != 0u8;

        let packet = match_packet(packet_id);
        gs_assert!(packet.is_some(), "invalid packet was sent");

        let mut packet = packet.unwrap();
        if packet.get_encrypted() && !encrypted {
            gs_assert!(
                false,
                "client sent a cleartext packet when expected an encrypted one"
            );
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

    /* public api for the main server */

    pub fn new(peer: SocketAddr, socket: Arc<UdpSocket>, secret_key: SecretKey) -> Self {
        let (tx, rx) = mpsc::channel::<Vec<u8>>(4);
        Self {
            tx,
            rx: Mutex::new(rx),
            peer,
            secret_key,
            socket,
            crypto_box: Mutex::new(None),
        }
    }

    pub async fn run(&self) -> anyhow::Result<()> {
        let mut rx = self.rx.lock().await;
        while let Some(message) = rx.recv().await {
            match self.parse_packet(message).await {
                Ok(packet) => match self.handle_packet(packet).await {
                    Ok(_) => {}
                    Err(err) => warn!("failed to handle packet: {}", err.to_string()),
                },
                Err(err) => warn!("failed to parse packet: {}", err.to_string()),
            }
        }

        Ok(())
    }

    pub async fn send_packet(&self, data: Vec<u8>) -> anyhow::Result<()> {
        self.tx.send(data).await?;
        Ok(())
    }
}
