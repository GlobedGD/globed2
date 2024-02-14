use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 10000)]
pub struct PingPacket {
    pub id: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 10001)]
pub struct CryptoHandshakeStartPacket {
    pub protocol: u16,
    pub key: CryptoPublicKey,
}

#[derive(Packet, Decodable)]
#[packet(id = 10002)]
pub struct KeepalivePacket;

pub const MAX_TOKEN_SIZE: usize = 164;
#[derive(Packet, Decodable)]
#[packet(id = 10003, encrypted = true)]
pub struct LoginPacket {
    pub secret_key: u32,
    pub account_id: i32,
    pub user_id: i32,
    pub name: FastString<MAX_NAME_SIZE>,
    pub token: FastString<MAX_TOKEN_SIZE>,
    pub icons: PlayerIconData,
    pub fragmentation_limit: u16,
}

#[derive(Packet, Decodable)]
#[packet(id = 10004)]
pub struct DisconnectPacket;

#[derive(Packet, Decodable)]
#[packet(id = 10005)]
pub struct ClaimThreadPacket {
    pub secret_key: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 10006)]
pub struct KeepaliveTCPPacket;

#[derive(Packet, Decodable)]
#[packet(id = 10010)]
pub struct ConnectionTestPacket {
    pub uid: u32,
    pub data: Vec<u8>,
}
