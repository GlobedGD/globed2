use std::collections::{HashMap, HashSet};

use serde::{Deserialize, Serialize};

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
    pub no_chat: HashSet<i32>,
    pub special_users: HashMap<i32, SpecialUser>,
}
