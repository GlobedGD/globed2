use std::{
    fmt::Display,
    io,
    net::SocketAddrV4,
    sync::{
        atomic::{AtomicBool, AtomicI32, Ordering},
        Arc,
    },
    time::{Duration, SystemTime, SystemTimeError},
};

use parking_lot::Mutex as SyncMutex;

use bytebuffer::{ByteBuffer, ByteReader};
use crypto_box::{
    aead::{Aead, AeadCore, OsRng},
    ChaChaBox,
};
use globed_shared::PROTOCOL_VERSION;
use log::{debug, warn};
use tokio::sync::{mpsc, Mutex};

use crate::{
    bytebufferext::*,
    data::{
        packets::{client::*, server::*, Packet, PacketMetadata, PACKET_HEADER_LEN},
        types::*,
    },
    server::GameServer,
};

// TODO adjust this to PlayerData size in the future plus some headroom
pub const SMALL_PACKET_LIMIT: usize = 128;
const CHANNEL_BUFFER_SIZE: usize = 8;

pub enum ServerThreadMessage {
    Packet(Vec<u8>),
    SmallPacket([u8; SMALL_PACKET_LIMIT]),
    BroadcastVoice(Arc<VoiceBroadcastPacket>),
    TerminationNotice(String),
}

pub struct GameServerThread {
    game_server: &'static GameServer,

    rx: Mutex<mpsc::Receiver<ServerThreadMessage>>,
    tx: mpsc::Sender<ServerThreadMessage>,
    awaiting_termination: AtomicBool,
    pub authenticated: AtomicBool,
    crypto_box: SyncMutex<Option<ChaChaBox>>,

    peer: SocketAddrV4,
    pub account_id: AtomicI32,
    pub level_id: AtomicI32,
    pub account_data: SyncMutex<PlayerAccountData>,

    last_voice_packet: SyncMutex<SystemTime>,
}

macro_rules! gs_handler {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        async fn $name(&$self, buf: &mut ByteReader<'_>) -> Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!("Handling packet {}", <$pktty>::NAME);
            $code
        }
    };
}

macro_rules! gs_handler_sync {
    ($self:ident,$name:ident,$pktty:ty,$pkt:ident,$code:expr) => {
        fn $name(&$self, buf: &mut ByteReader<'_>) -> Result<()> {
            let $pkt = <$pktty>::decode_from_reader(buf)?;
            #[cfg(debug_assertions)]
            log::debug!("Handling packet {}", <$pktty>::NAME);
            $code
        }
    };
}

macro_rules! gs_disconnect {
    ($self:ident, $msg:expr) => {
        $self.terminate();
        $self
            .send_packet_fast(&ServerDisconnectPacket { message: $msg })
            .await?;
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
            gs_disconnect!($self, "unauthorized, please try connecting again".to_string());
        }
    };
}

enum PacketHandlingError {
    Other(String),
    WrongCryptoBoxState,
    EncryptionError(String),
    DecryptionError(String),
    IOError(io::Error),
    MalformedMessage,
    MalformedLoginAttempt,
    MalformedCiphertext,
    NoHandler(u16),
    WebRequestError(reqwest::Error),
    UnexpectedPlayerData,
    SystemTimeError(SystemTimeError),
    SocketSendFailed(io::Error),
    SocketWouldBlock,
    UnexpectedCentralResponse,
}

type Result<T> = core::result::Result<T, PacketHandlingError>;

impl GameServerThread {
    /* public api for the main server */

    pub fn new(peer: SocketAddrV4, game_server: &'static GameServer) -> Self {
        let (tx, rx) = mpsc::channel::<ServerThreadMessage>(CHANNEL_BUFFER_SIZE);
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

    pub async fn run(&self) {
        let mut rx = self.rx.lock().await;

        loop {
            if self.awaiting_termination.load(Ordering::Relaxed) {
                break;
            }

            match tokio::time::timeout(Duration::from_secs(60), rx.recv()).await {
                Ok(Some(message)) => match self.handle_message(message).await {
                    Ok(()) => {}
                    Err(err) => warn!("[@{}]: {}", self.peer, err.to_string()),
                },
                Ok(None) | Err(_) => break, // sender closed | timeout
            };
        }
    }

    pub fn push_new_message(&self, data: ServerThreadMessage) -> anyhow::Result<()> {
        self.tx.try_send(data)?;
        Ok(())
    }

    pub fn terminate(&self) {
        self.awaiting_termination.store(true, Ordering::Relaxed);
    }

    /* private utilities */

    async fn send_packet<P: Packet>(&self, packet: &P) -> Result<()> {
        #[cfg(debug_assertions)]
        log::debug!("Sending {}", P::NAME);

        let serialized = self.serialize_packet(packet)?;
        self.send_buffer(serialized.as_bytes()).await
    }

    // fast packet sending with zero heap allocation
    // packet must implement EncodableWithKnownSize to be fast sendable
    // it also must **NOT** be encrypted.
    // on average 2-3x faster than send_packet, even worst case should be faster by a bit
    async fn send_packet_fast<P: Packet + EncodableWithKnownSize>(&self, packet: &P) -> Result<()> {
        assert!(
            !packet.get_encrypted(),
            "Attempting to fast encode an encrypted packet ({})",
            P::NAME
        );

        let to_send: Result<Option<Vec<u8>>> = alloca::with_alloca(PACKET_HEADER_LEN + P::ENCODED_SIZE, move |data| {
            // safety: 'data' will have garbage data but that is considered safe for all our intents and purposes
            // as `FastByteBuffer::as_bytes()` will only return what was already written.
            let data = unsafe {
                let ptr = data.as_mut_ptr().cast::<u8>();
                let len = std::mem::size_of_val(data);
                std::slice::from_raw_parts_mut(ptr, len)
            };

            let mut buf = FastByteBuffer::new(data);
            buf.write_u16(packet.get_packet_id());
            buf.write_bool(false);
            buf.write_value(packet);

            let send_data = buf.as_bytes();
            match self.send_buffer_immediate(send_data) {
                Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(send_data.to_vec())),
                Err(e) => Err(e),
                Ok(()) => Ok(None),
            }
        });

        if let Some(to_send) = to_send? {
            self.send_buffer(&to_send).await?;
        }

        Ok(())
    }

    async fn send_buffer(&self, buffer: &[u8]) -> Result<()> {
        self.game_server
            .socket
            .send_to(buffer, self.peer)
            .await
            .map(|_size| ())
            .map_err(PacketHandlingError::SocketSendFailed)
    }

    // attempt to send a buffer immediately to the socket, but if it requires blocking then nuh uh
    fn send_buffer_immediate(&self, buffer: &[u8]) -> Result<()> {
        self.game_server
            .socket
            .try_send_to(buffer, std::net::SocketAddr::V4(self.peer))
            .map(|_| ())
            .map_err(|e| {
                if e.kind() == std::io::ErrorKind::WouldBlock {
                    PacketHandlingError::SocketWouldBlock
                } else {
                    PacketHandlingError::SocketSendFailed(e)
                }
            })
    }

    fn serialize_packet<P: Packet>(&self, packet: &P) -> Result<ByteBuffer> {
        let mut buf = ByteBuffer::new();
        buf.write_u16(P::PACKET_ID);
        buf.write_bool(P::ENCRYPTED);

        if !P::ENCRYPTED {
            packet.encode(&mut buf);
            return Ok(buf);
        }

        let cbox = self.crypto_box.lock();

        // should never happen
        #[cfg(debug_assertions)]
        if !cbox.is_some() {
            return Err(PacketHandlingError::WrongCryptoBoxState);
        }

        let mut cltxtbuf = ByteBuffer::new();
        packet.encode(&mut cltxtbuf);

        let cbox = cbox.as_ref().unwrap();
        let nonce = ChaChaBox::generate_nonce(&mut OsRng);

        let encrypted = cbox
            .encrypt(&nonce, cltxtbuf.as_bytes())
            .map_err(|e| PacketHandlingError::EncryptionError(e.to_string()))?;

        #[cfg(not(rust_analyzer))] // i am so sorry
        buf.write_bytes(&nonce);
        buf.write_bytes(&encrypted);

        Ok(buf)
    }

    async fn handle_message(&self, message: ServerThreadMessage) -> Result<()> {
        match message {
            ServerThreadMessage::Packet(data) => self.handle_packet(&data).await?,
            ServerThreadMessage::SmallPacket(data) => self.handle_packet(&data).await?,
            ServerThreadMessage::BroadcastVoice(voice_packet) => self.send_packet(&*voice_packet).await?,
            ServerThreadMessage::TerminationNotice(message) => {
                self.terminate();
                self.send_packet(&ServerDisconnectPacket { message }).await?;
            }
        }

        Ok(())
    }

    /* packet handlers */

    async fn handle_packet(&self, message: &[u8]) -> Result<()> {
        #[cfg(debug_assertions)]
        if message.len() < PACKET_HEADER_LEN {
            return Err(PacketHandlingError::MalformedMessage);
        }

        let mut data = ByteReader::from_bytes(message);

        let packet_id = data.read_u16()?;
        let encrypted = data.read_bool()?;

        // minor optimization
        if packet_id == PlayerDataPacket::PACKET_ID {
            return self.handle_player_data(&mut data).await;
        }

        // also for optimization, reject the voice packet immediately if the player is blocked from vc
        if packet_id == VoicePacket::PACKET_ID {
            let accid = self.account_id.load(Ordering::Relaxed);
            if self.game_server.chat_blocked(accid) {
                debug!("blocking voice packet from {accid}");
                return Ok(());
            }
        }

        // reject cleartext credentials
        if packet_id == LoginPacket::PACKET_ID && !encrypted {
            return Err(PacketHandlingError::MalformedLoginAttempt);
        }

        let cleartext_vec: Vec<u8>;
        if encrypted {
            if message.len() < 24 + PACKET_HEADER_LEN {
                return Err(PacketHandlingError::MalformedCiphertext);
            }

            let cbox = self.crypto_box.lock();
            if cbox.is_none() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let cbox = cbox.as_ref().unwrap();

            let nonce = &message[PACKET_HEADER_LEN..PACKET_HEADER_LEN + 24];
            let ciphertext = &message[PACKET_HEADER_LEN + 24..];

            cleartext_vec = cbox
                .decrypt(nonce.into(), ciphertext)
                .map_err(|e| PacketHandlingError::DecryptionError(e.to_string()))?;
            data = ByteReader::from_bytes(&cleartext_vec);
        }

        match packet_id {
            /* connection related */
            PingPacket::PACKET_ID => self.handle_ping(&mut data).await,
            CryptoHandshakeStartPacket::PACKET_ID => self.handle_crypto_handshake(&mut data).await,
            KeepalivePacket::PACKET_ID => self.handle_keepalive(&mut data).await,
            LoginPacket::PACKET_ID => self.handle_login(&mut data).await,
            DisconnectPacket::PACKET_ID => self.handle_disconnect(&mut data),

            /* game related */
            SyncIconsPacket::PACKET_ID => self.handle_sync_icons(&mut data).await,
            RequestProfilesPacket::PACKET_ID => self.handle_request_profiles(&mut data).await,
            LevelJoinPacket::PACKET_ID => self.handle_level_join(&mut data).await,
            LevelLeavePacket::PACKET_ID => self.handle_level_leave(&mut data).await,
            PlayerDataPacket::PACKET_ID => self.handle_player_data(&mut data).await,
            RequestPlayerListPacket::PACKET_ID => self.handle_request_player_list(&mut data).await,

            VoicePacket::PACKET_ID => self.handle_voice(&mut data).await,
            x => Err(PacketHandlingError::NoHandler(x)),
        }
    }

    /* connection related */

    gs_handler!(self, handle_ping, PingPacket, packet, {
        self.send_packet_fast(&PingResponsePacket {
            id: packet.id,
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
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
            if cbox.is_some() {
                return Err(PacketHandlingError::WrongCryptoBoxState);
            }

            let new_box = ChaChaBox::new(&packet.key.pubkey, &self.game_server.secret_key);
            *cbox = Some(new_box);
        }

        self.send_packet_fast(&CryptoHandshakeResponsePacket {
            key: CryptoPublicKey {
                pubkey: self.game_server.secret_key.public_key().clone(),
            },
        })
        .await
    });

    gs_handler!(self, handle_keepalive, KeepalivePacket, _packet, {
        gs_needauth!(self);

        self.send_packet_fast(&KeepaliveResponsePacket {
            player_count: self.game_server.state.player_count.load(Ordering::Relaxed),
        })
        .await
    });

    gs_handler!(self, handle_login, LoginPacket, packet, {
        if self.game_server.standalone {
            debug!("Bypassing login for {}", packet.account_id);
            self.game_server.check_already_logged_in(packet.account_id)?;
            self.authenticated.store(true, Ordering::Relaxed);
            self.account_id.store(packet.account_id, Ordering::Relaxed);
            self.game_server.state.player_count.fetch_add(1u32, Ordering::Relaxed);
            {
                let mut account_data = self.account_data.lock();
                account_data.account_id = packet.account_id;
                account_data.name = format!("Player{}", packet.account_id);
            }
            self.send_packet_fast(&LoggedInPacket {}).await?;
            return Ok(());
        }

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
                message: format!("authentication failed: {response}"),
            })
            .await?;

            return Ok(());
        }

        let player_name = response
            .split_once(':')
            .ok_or(PacketHandlingError::UnexpectedCentralResponse)?
            .1;

        self.game_server.check_already_logged_in(packet.account_id)?;
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

        self.send_packet_fast(&LoggedInPacket {}).await?;

        Ok(())
    });

    gs_handler_sync!(self, handle_disconnect, DisconnectPacket, _packet, {
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
            profiles: self.game_server.gather_profiles(&packet.ids),
        })
        .await
    });

    gs_handler!(self, handle_level_join, LevelJoinPacket, packet, {
        gs_needauth!(self);

        let account_id = self.account_id.load(Ordering::Relaxed);
        let old_level = self.level_id.swap(packet.level_id, Ordering::Relaxed);

        let mut pm = self.game_server.state.player_manager.lock();

        if old_level != 0 {
            pm.remove_from_level(old_level, account_id);
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
            return Err(PacketHandlingError::UnexpectedPlayerData);
        }

        let account_id = self.account_id.load(Ordering::Relaxed);

        let retval: Result<Option<Vec<u8>>> = {
            let mut pm = self.game_server.state.player_manager.lock();
            pm.set_player_data(account_id, &packet.data);

            // this unwrap should be safe
            let written_players = pm.get_player_count_on_level(level_id).unwrap() - 1;
            drop(pm);

            let calc_size = PACKET_HEADER_LEN + 4 + (written_players * AssociatedPlayerData::ENCODED_SIZE);

            alloca::with_alloca(calc_size, move |data| {
                // safety: 'data' will have garbage data but that is considered safe for all our intents and purposes
                // as `FastByteBuffer::as_bytes()` will only return what was already written.
                let data = unsafe {
                    let ptr = data.as_mut_ptr().cast::<u8>();
                    let len = std::mem::size_of_val(data);
                    std::slice::from_raw_parts_mut(ptr, len)
                };

                let mut buf = FastByteBuffer::new(data);

                // dont actually do this anywhere else please
                buf.write_u16(LevelDataPacket::PACKET_ID);
                buf.write_bool(false);
                buf.write_u32(written_players as u32);

                // this is scary
                self.game_server.state.player_manager.lock().for_each_player_on_level(
                    level_id,
                    |player, buf| {
                        // we do additional length check because player count may have changed since 1st lock
                        // NOTE: this assumes encoded size of AssociatedPlayerData is a constant
                        // change this to something else if that won't be true in the future
                        if buf.len() != buf.capacity() && player.account_id != account_id {
                            buf.write_value(player);
                        }
                    },
                    &mut buf,
                );

                let data = buf.as_bytes();
                match self.send_buffer_immediate(data) {
                    // if we cant send without blocking, accept our defeat and clone the data to a vec
                    Err(PacketHandlingError::SocketWouldBlock) => Ok(Some(data.to_vec())),
                    // if another error occured, propagate it up
                    Err(e) => Err(e),
                    // if all good, do nothing
                    Ok(()) => Ok(None),
                }
            })
        };

        if let Some(data) = retval? {
            debug!("fast PlayerData response failed, issuing a blocking call");
            self.send_buffer(&data).await?;
        }

        Ok(())
    });

    gs_handler!(self, handle_request_player_list, RequestPlayerListPacket, _packet, {
        gs_needauth!(self);

        self.send_packet(&PlayerListPacket {
            profiles: self.game_server.gather_all_profiles(),
        })
        .await
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

            let total_size = packet.data.opus_frames.iter().map(Vec::len).sum::<usize>();

            let throughput = total_size / passed_time as usize; // in kb/s

            debug!("voice packet throughput: {}kb/s", throughput);
            if throughput > 8 {
                warn!("rejecting a voice packet, throughput above the limit: {}kb/s", throughput);
                return Ok(());
            }
        }

        let vpkt = Arc::new(VoiceBroadcastPacket {
            player_id: accid,
            data: packet.data.clone(),
        });

        self.game_server.broadcast_voice_packet(&vpkt)?;

        Ok(())
    });
}

impl From<anyhow::Error> for PacketHandlingError {
    fn from(value: anyhow::Error) -> Self {
        PacketHandlingError::Other(value.to_string())
    }
}

impl From<reqwest::Error> for PacketHandlingError {
    fn from(value: reqwest::Error) -> Self {
        PacketHandlingError::WebRequestError(value)
    }
}

impl From<SystemTimeError> for PacketHandlingError {
    fn from(value: SystemTimeError) -> Self {
        PacketHandlingError::SystemTimeError(value)
    }
}

impl From<std::io::Error> for PacketHandlingError {
    fn from(value: std::io::Error) -> Self {
        PacketHandlingError::IOError(value)
    }
}

impl Display for PacketHandlingError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Other(msg) => f.write_str(msg),
            Self::IOError(msg) => f.write_fmt(format_args!("IO Error: {msg}")),
            Self::WrongCryptoBoxState => f.write_str("wrong crypto box state for the given operation"),
            Self::EncryptionError(msg) => f.write_fmt(format_args!("Encryption failed: {msg}")),
            Self::DecryptionError(msg) => f.write_fmt(format_args!("Decryption failed: {msg}")),
            Self::MalformedCiphertext => f.write_str("malformed ciphertext in an encrypted packet"),
            Self::MalformedMessage => f.write_str("malformed message structure"),
            Self::MalformedLoginAttempt => f.write_str("malformed login attempt"),
            Self::NoHandler(id) => f.write_fmt(format_args!("no packet handler for packet ID {id}")),
            Self::WebRequestError(msg) => f.write_fmt(format_args!("web request error: {msg}")),
            Self::UnexpectedPlayerData => f.write_str("received PlayerDataPacket when not on a level"),
            Self::SystemTimeError(msg) => f.write_fmt(format_args!("system time error: {msg}")),
            Self::SocketSendFailed(err) => f.write_fmt(format_args!("socket send failed: {err}")),
            Self::SocketWouldBlock => f.write_str("could not do a non-blocking operation on the socket as it would block"),
            Self::UnexpectedCentralResponse => f.write_str("got unexpected response from the central server"),
        }
    }
}
