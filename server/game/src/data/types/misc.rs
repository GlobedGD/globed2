use crate::data::*;

#[derive(Encodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct GlobedLevel {
    pub level_id: LevelId,
    pub player_count: u16,
}
