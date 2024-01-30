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

/* SpecificIconData (specific player data) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct SpecificIconData {
    pub position: Point,
    pub rotation: f32,
    pub icon_type: PlayerIconType,
    pub flag_byte: u8,
}

/* PlayerData (data in a level) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct PlayerData {
    pub timestamp: f32,
    pub percentage: u16,
    pub attempts: i32,

    pub player1: SpecificIconData,
    pub player2: SpecificIconData,

    pub last_death_timestamp: f32,

    pub flag_byte: u8,
}
