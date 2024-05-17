use crate::data::*;

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

#[derive(Packet, Encodable, StaticSize, DynamicSize)]
#[packet(id = 20004, tcp = true)]
pub struct LoggedInPacket {
    pub tps: u32,
    pub special_user_data: Option<SpecialUserData>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20005, tcp = true)]
pub struct LoginFailedPacket<'a> {
    pub message: &'a str,
}

// used to communicate a simple message to the user
#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20006, tcp = false)]
pub struct ServerNoticePacket {
    pub message: FastString,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20007, tcp = true)]
pub struct ProtocolMismatchPacket {
    pub protocol: u16,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20008, tcp = true)]
pub struct KeepaliveTCPResponsePacket;

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20010, tcp = false)]
pub struct ConnectionTestResponsePacket {
    pub uid: u32,
    pub data: Vec<u8>,
}

#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20011, tcp = true)]
pub struct ServerBannedPacket {
    pub message: FastString,
    pub timestamp: i64,
}

#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20012)]
pub struct ServerMutedPacket {
    pub reason: FastString,
    pub timestamp: i64,
}
