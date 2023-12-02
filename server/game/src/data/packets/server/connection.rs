use crate::data::*;

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20000, encrypted = false)]
pub struct PingResponsePacket {
    pub id: u32,
    pub player_count: u32,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20001, encrypted = false)]
pub struct CryptoHandshakeResponsePacket {
    pub key: PublicKey,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20002, encrypted = false)]
pub struct KeepaliveResponsePacket {
    pub player_count: u32,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20003, encrypted = false)]
pub struct ServerDisconnectPacket {
    pub message: FastString<MAX_NOTICE_SIZE>,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20004, encrypted = false)]
pub struct LoggedInPacket {
    pub tps: u32,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20005, encrypted = false)]
pub struct LoginFailedPacket {
    pub message: FastString<MAX_NOTICE_SIZE>,
}

// used to communicate a simple message to the user
#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 20006, encrypted = false)]
pub struct ServerNoticePacket {
    pub message: FastString<MAX_NOTICE_SIZE>,
}
