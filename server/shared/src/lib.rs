use serde::{Deserialize, Serialize};

pub const PROTOCOL_VERSION: u16 = 1;

#[derive(Serialize, Deserialize)]
pub struct GameServerBootData {
    pub protocol: u16,
    pub no_chat: Vec<i32>,
}
