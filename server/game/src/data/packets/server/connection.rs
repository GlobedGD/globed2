use crate::data::*;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20000)]
pub struct PingResponsePacket {
    pub id: u32,
    pub player_count: u32,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20001)]
pub struct CryptoHandshakeResponsePacket {
    pub key: CryptoPublicKey,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20002)]
pub struct KeepaliveResponsePacket {
    pub player_count: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20003)]
pub struct ServerDisconnectPacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20004)]
pub struct LoggedInPacket {
    pub tps: u32,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20005)]
pub struct LoginFailedPacket<'a> {
    pub message: &'a str,
}

// used to communicate a simple message to the user
#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20006)]
pub struct ServerNoticePacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 20007)]
pub struct ProtocolMismatchPacket {
    pub protocol: u16,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 20010)]
pub struct ConnectionTestResponsePacket {
    pub uid: u32,
    pub data: Vec<u8>,
}
