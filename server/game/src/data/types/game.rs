use crate::data::*;

/* PlayerIconType */

#[derive(Default, Debug, Copy, Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
#[repr(u8)]
pub enum PlayerIconType {
    #[default]
    Unknown = 0,
    Cube = 1,
    Ship = 2,
    Ball = 3,
    Ufo = 4,
    Wave = 5,
    Robot = 6,
    Spider = 7,
    Swing = 8,
}

/* SpiderTeleportData (spider teleport data) */
#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct SpiderTeleportData {
    pub from: Point,
    pub to: Point,
}

/* SpecificIconData (specific player data) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct SpecificIconData {
    pub position: Point,
    pub rotation: f32,
    pub icon_type: PlayerIconType,
    pub flags: u16, // bit-field with various flags, see the client-side structure for more info
    pub spider_teleport_data: Option<SpiderTeleportData>,
}

/* PlayerData (data in a level) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct PlayerData {
    pub timestamp: f32,
    pub local_best: u32, // percentage or milliseconds in platformer
    pub attempts: i32,

    pub player1: SpecificIconData,
    pub player2: SpecificIconData,

    pub last_death_timestamp: f32,

    pub current_percentage: f32,

    pub flags: u8, // also a bit-field
}
