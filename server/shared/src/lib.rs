use serde::{Deserialize, Serialize};

pub const PROTOCOL_VERSION: u32 = 1;

#[derive(Serialize, Deserialize)]
pub struct GameServerBootData {
    pub protocol: u32,
}
