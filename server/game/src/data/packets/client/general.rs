use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 11000, encrypted = false)]
pub struct SyncIconsPacket {
    pub icons: PlayerIconData,
}

#[derive(Packet, Decodable)]
#[packet(id = 11001, encrypted = false)]
pub struct RequestGlobalPlayerListPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11002, encrypted = false)]
pub struct CreateRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11003, encrypted = false)]
pub struct JoinRoomPacket {
    pub room_id: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 11004, encrypted = false)]
pub struct LeaveRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11005, encrypted = false)]
pub struct RequestRoomPlayerListPacket;
