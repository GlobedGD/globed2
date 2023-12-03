use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 10000, encrypted = false)]
pub struct PingPacket {
    pub id: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 10001, encrypted = false)]
pub struct CryptoHandshakeStartPacket {
    pub protocol: u16,
    pub key: PublicKey,
}

#[derive(Packet, Decodable)]
#[packet(id = 10002, encrypted = false)]
pub struct KeepalivePacket;

pub const MAX_TOKEN_SIZE: usize = 164;
#[derive(Packet, Decodable)]
#[packet(id = 10003, encrypted = true)]
pub struct LoginPacket {
    pub account_id: i32,
    pub name: FastString<MAX_NAME_SIZE>,
    pub token: FastString<MAX_TOKEN_SIZE>,
}

#[derive(Packet, Decodable)]
#[packet(id = 10004, encrypted = false)]
pub struct DisconnectPacket;
