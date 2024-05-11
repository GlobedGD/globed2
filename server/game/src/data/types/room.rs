use crate::data::*;

#[derive(Clone, Copy, Default, Encodable, Decodable, StaticSize, DynamicSize, Debug)]
#[bitfield(on = true, size = 2)]
#[allow(clippy::struct_excessive_bools)]
pub struct RoomSettingsFlags {
    pub invite_only: bool,
    pub public_invites: bool,
    pub collision: bool,
    pub two_player: bool,
}

#[derive(Clone, Copy, Default, Encodable, Decodable, StaticSize, DynamicSize, Debug)]
#[dynamic_size(as_static = true)]
pub struct RoomSettings {
    pub flags: RoomSettingsFlags,
    pub reserved: u64, // space for future expansions, should be always 0
}

#[derive(Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomInfo {
    pub id: u32,
    pub owner: i32,
    pub token: u32,
    pub settings: RoomSettings,
}

#[derive(Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomListingInfo {
    pub id: u32,
    pub owner: i32,
    pub settings: RoomSettings,
}
