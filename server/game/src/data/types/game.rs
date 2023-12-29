use crate::data::*;

/* PlayerIconType */

#[derive(Debug, Copy, Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
#[repr(u8)]
pub enum PlayerIconType {
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
    pub icon_type: PlayerIconType,
    pub position: Point,
    pub rotation: f32,
}

/* PlayerData (data in a level) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct PlayerData {
    pub percentage: u16,
    pub attempts: i32,
}
