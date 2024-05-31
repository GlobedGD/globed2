use crate::{data::*, managers::GameServerRole};

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20000, tcp = false)]
pub struct PingResponsePacket {
    pub id: u32,
    pub player_count: u32,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20001, tcp = true)]
pub struct CryptoHandshakeResponsePacket {
    pub key: CryptoPublicKey,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20002, tcp = false)]
pub struct KeepaliveResponsePacket {
    pub player_count: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20003, tcp = true)]
pub struct ServerDisconnectPacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20004, encrypted = true, tcp = true)]
pub struct LoggedInPacket {
    pub tps: u32,
    pub special_user_data: SpecialUserData,
    pub all_roles: Vec<GameServerRole>,
    pub secret_key: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20005, tcp = true)]
pub struct LoginFailedPacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20006, tcp = true)]
pub struct ProtocolMismatchPacket<'a> {
    pub protocol: u16,
    pub min_client_version: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20007, tcp = true)]
pub struct KeepaliveTCPResponsePacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20008, tcp = false)]
pub struct ClaimThreadFailedPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20009, tcp = true)]
pub struct LoginRecoveryFailedPacket;

// used to communicate a simple message to the user
#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20100, tcp = false)]
pub struct ServerNoticePacket {
    pub message: FastString,
}

#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20101, tcp = true)]
pub struct ServerBannedPacket {
    pub message: FastString,
    pub timestamp: i64,
}

#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20102)]
pub struct ServerMutedPacket {
    pub reason: FastString,
    pub timestamp: i64,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20200, tcp = false)]
pub struct ConnectionTestResponsePacket {
    pub uid: u32,
    pub data: Vec<u8>,
}
