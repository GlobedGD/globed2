use crate::data::*;

/*
* For optimization reasons, most of those packets are encoded inline, and their structure is not present here.
*/

#[derive(Packet, Encodable)]
#[packet(id = 21000, tcp = true)]
pub struct GlobalPlayerListPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21001, tcp = false)]
pub struct RoomCreatedPacket {
    pub room_id: u32,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21002, tcp = false)]
pub struct RoomJoinedPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21003, tcp = false)]
pub struct RoomJoinFailedPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21004, tcp = true)]
pub struct RoomPlayerListPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21005, tcp = true)]
pub struct LevelListPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21006)]
pub struct LevelPlayerCountPacket {
    pub level_id: i32,
    pub player_count: u16,
}
