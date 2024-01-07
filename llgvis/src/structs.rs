use esp::*;
use globed_derive::*;

#[derive(Decodable, Clone)]
pub struct PlayerLogData {
    pub local_timestamp: f32,
    pub timestamp: f32,
    pub position: (f32, f32),
    pub rotation: f32,
}

#[derive(Decodable, Clone)]
pub struct PlayerLog {
    pub real: Vec<PlayerLogData>,
    pub real_extrapolated: Vec<(PlayerLogData, PlayerLogData)>,
    pub lerped: Vec<PlayerLogData>,
    pub lerp_skipped: Vec<PlayerLogData>,
}
