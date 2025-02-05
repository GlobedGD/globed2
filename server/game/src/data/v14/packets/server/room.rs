use crate::data::*;
use std::borrow::Cow;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 23000, tcp = false)]
pub struct RoomCreatedPacket {
    pub info: RoomInfo,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 23001, tcp = true)]
pub struct RoomJoinedPacket {
    pub room_info: RoomInfo,
    pub players: Vec<PlayerRoomPreviewAccountData>,
}

#[derive(Packet, Encodable, StaticSize, Default)]
#[packet(id = 23002, tcp = false)]
pub struct RoomJoinFailedPacket {
    pub was_invalid: bool,
    pub was_protected: bool,
    pub was_full: bool,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 23003, tcp = true)]
pub struct RoomPlayerListPacket {
    pub room_info: RoomInfo,
    pub players: Vec<PlayerRoomPreviewAccountData>,
}

#[derive(Packet, Encodable, StaticSize, DynamicSize, Clone)]
#[packet(id = 23004)]
pub struct RoomInfoPacket {
    pub info: RoomInfo,
}

#[derive(Packet, Encodable, StaticSize, DynamicSize, Clone)]
#[packet(id = 23005)]
pub struct RoomInvitePacket {
    pub player_data: PlayerPreviewAccountData,
    pub room_id: u32,
    pub room_password: InlineString<16>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 23006, tcp = true)]
pub struct RoomListPacket {
    pub rooms: Vec<RoomListingInfo>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 23007)]
pub struct RoomCreateFailedPacket<'a> {
    pub reason: Cow<'a, str>,
}
