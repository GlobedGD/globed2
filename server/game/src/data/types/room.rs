use crate::data::*;

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomSettings {
    pub flags: Bits<2>,
    pub reserved: u64, // space for future expansions, should be always 0
}

#[derive(Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomInfo {
    pub id: u32,
    pub owner: i32,
    pub settings: RoomSettings,
}
