use crate::data::*;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 23000, tcp = false)]
pub struct RoomCreatedPacket {
    pub info: RoomInfo,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 23001, tcp = false)]
pub struct RoomJoinedPacket;

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
    pub player_data: PlayerRoomPreviewAccountData,
    pub room_id: u32,
    pub room_password: InlineString<16>,
}

#[derive(Packet, Encodable)]
#[packet(id = 23006)]
pub struct RoomListPacket {
    rooms: Vec<RoomListingInfo>,
}
