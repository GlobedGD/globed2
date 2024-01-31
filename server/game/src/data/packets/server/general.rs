use crate::data::*;

/*
* For optimization reasons, `PlayerListPacket` and `RoomPlayerListPacket` are encoded inline,
* their structure is not present here.
*/

#[derive(Packet, Encodable)]
#[packet(id = 21000)]
pub struct GlobalPlayerListPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21001)]
pub struct RoomCreatedPacket {
    pub room_id: u32,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21002)]
pub struct RoomJoinedPacket;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 21003)]
pub struct RoomJoinFailedPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21004)]
pub struct RoomPlayerListPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21005)]
pub struct LevelListPacket;
