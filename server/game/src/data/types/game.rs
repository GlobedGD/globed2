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
    Jetpack = 9,
}

/* SpiderTeleportData (spider teleport data) */
#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct SpiderTeleportData {
    pub from: Point,
    pub to: Point,
}

/* SpecificIconData (specific player data) */
// 16 bytes best-case, 32 bytes worst-case (when on the same frame as spider TP).

#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
pub struct SpecificIconData {
    pub position: Point,
    pub rotation: FiniteF32,
    pub y_vel: FiniteF32,
    pub fall_speed: FiniteF32,
    pub icon_type: PlayerIconType,
    pub flags: Bits<2>, // bit-field with various flags, see the client-side structure for more info
    pub spider_teleport_data: Option<SpiderTeleportData>,
}

/* PlayerMetadata (player things that are sent less often) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct PlayerMetadata {
    pub local_best: u32, // percentage or milliseconds in platformer
    pub attempts: i32,
}

/* PlayerData (data in a level) */
// 45 bytes best-case, 77 bytes worst-case (with 2 spider teleports).

#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
pub struct PlayerData {
    pub timestamp: FiniteF32,
    pub np_timestamp: FiniteF32, // non persistent timestamp, resets every attempt

    pub player1: SpecificIconData,
    pub player2: SpecificIconData,

    pub last_death_timestamp: FiniteF32,

    pub current_percentage: FiniteF32,

    pub flags: Bits<1>, // also a bit-field
}

#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
pub struct SwitchData {
    pub player: i32,
    pub timestamp: FiniteF32,
}
