use crate::data::*;

/*
* For optimization reasons, this packet is encoded inline in the packet handler in server_thread/handlers/general.rs.
*/

#[derive(Packet, Encodable)]
#[packet(id = 21000, encrypted = false)]
pub struct PlayerListPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21001, encrypted = false)]
pub struct RoomCreatedPacket {
    room_id: FastString<ROOM_ID_LENGTH>,
}

#[derive(Packet, Encodable)]
#[packet(id = 21002, encrypted = false)]
pub struct RoomJoinedPacket;

#[derive(Packet, Encodable)]
#[packet(id = 21003, encrypted = false)]
pub struct RoomJoinFailedPacket;
