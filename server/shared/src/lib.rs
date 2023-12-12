use serde::{Deserialize, Serialize};

// import reexports
pub use nohash_hasher::IntMap;
// module reexports
pub use colored;
pub use time;
// our reexports
pub use logger::*;
pub mod logger;

pub const PROTOCOL_VERSION: u16 = 1;

fn default_su_color() -> String {
    "#ffffff".to_string()
}

#[derive(Serialize, Deserialize, Default, Clone)]
pub struct SpecialUser {
    pub name: String,
    #[serde(default = "default_su_color")]
    pub color: String,
}

#[derive(Serialize, Deserialize)]
pub struct GameServerBootData {
    pub protocol: u16,
    pub no_chat: Vec<i32>,
    pub special_users: IntMap<i32, SpecialUser>,
    pub tps: u32,
    pub maintenance: bool,
}

impl Default for GameServerBootData {
    fn default() -> Self {
        Self {
            protocol: PROTOCOL_VERSION,
            no_chat: Vec::new(),
            special_users: IntMap::default(),
            tps: 30,
            maintenance: false,
        }
    }
}
