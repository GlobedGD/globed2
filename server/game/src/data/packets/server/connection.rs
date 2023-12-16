use crate::data::*;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20000, encrypted = false)]
pub struct PingResponsePacket {
    pub id: u32,
    pub player_count: u32,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20001, encrypted = false)]
pub struct CryptoHandshakeResponsePacket {
    pub key: CryptoPublicKey,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20002, encrypted = false)]
pub struct KeepaliveResponsePacket {
    pub player_count: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20003, encrypted = false)]
pub struct ServerDisconnectPacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20004, encrypted = false)]
pub struct LoggedInPacket {
    pub tps: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20005, encrypted = false)]
pub struct LoginFailedPacket<'a> {
    pub message: &'a str,
}

// used to communicate a simple message to the user
#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20006, encrypted = false)]
pub struct ServerNoticePacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20007, encrypted = false)]
pub struct ProtocolMismatchPacket {
    pub protocol: u16,
}
