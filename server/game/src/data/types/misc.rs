use crate::data::*;

#[derive(Encodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct GlobedLevel {
    pub level_id: i32,
    pub player_count: u16,
}
