pub mod audio_frame;
pub mod cocos;
pub mod crypto;
pub mod game;
pub mod gd;
pub mod misc;
pub mod room;
pub mod user;

use std::sync::atomic::AtomicI64;

pub use audio_frame::*;
pub use cocos::*;
pub use crypto::*;
pub use esp::types::*;
pub use game::*;
pub use gd::*;
pub use misc::*;
pub use room::*;
pub use user::*;
pub type LevelId = i64;
pub type AtomicLevelId = AtomicI64;

pub const fn is_editorcollab_level(id: LevelId) -> bool {
    // if changing back to i32 for whatever reason, remove this condition
    id > (2 as LevelId).pow(32)
}
