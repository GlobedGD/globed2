use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 13000)]
pub struct CreateRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 13001)]
pub struct JoinRoomPacket {
    pub room_id: u32,
    pub room_token: u32,
}

#[derive(Packet, Decodable)]
#[packet(id = 13002)]
pub struct LeaveRoomPacket;

#[derive(Packet, Decodable)]
#[packet(id = 13003)]
pub struct RequestRoomPlayerListPacket;

#[derive(Packet, Decodable)]
#[packet(id = 13004)]
pub struct UpdateRoomSettingsPacket {
    pub settings: RoomSettings,
}

#[derive(Packet, Decodable)]
#[packet(id = 13005)]
pub struct RoomSendInvitePacket {
    pub player: i32,
}

#[derive(Packet, Decodable)]
#[packet(id = 13006)]
pub struct RequestRoomListPacket;
