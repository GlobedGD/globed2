use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 11000)]
pub struct SyncIconsPacket {
    pub icons: PlayerIconData,
}

#[derive(Packet, Decodable)]
#[packet(id = 11001)]
pub struct RequestGlobalPlayerListPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11002)]
pub struct CreateRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11003)]
pub struct JoinRoomPacket {
    pub room_id: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 11004)]
pub struct LeaveRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11005)]
pub struct RequestRoomPlayerListPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11006)]
pub struct RequestLevelListPacket;

#[derive(Packet, Decodable)]
#[packet(id = 11007)]
pub struct RequestPlayerCountPacket {
    pub level_ids: FastVec<LevelId, 128>,
}
