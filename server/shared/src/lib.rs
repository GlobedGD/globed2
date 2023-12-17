#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc
)]

use esp::{Decodable, Encodable};
use globed_derive::{Decodable, Encodable};
use serde::{Deserialize, Serialize};

// import reexports
pub use nohash_hasher::IntMap;
// module reexports
pub use base64;
pub use colored;
pub use crypto_box;
pub use esp;
pub use hmac;
pub use sha2;
pub use time;
// our reexports
pub use logger::*;
pub use token_issuer::TokenIssuer;
pub mod logger;
pub mod token_issuer;

pub const PROTOCOL_VERSION: u16 = 1;
pub const SERVER_MAGIC: &[u8] = b"\xda\xeeglobed\xda\xee";
pub const SERVER_MAGIC_LEN: usize = SERVER_MAGIC.len();

fn default_su_color() -> String {
    "#ffffff".to_string()
}

#[derive(Encodable, Decodable, Serialize, Deserialize, Clone)]
pub struct SpecialUser {
    pub name: String,
    #[serde(default = "default_su_color")]
    pub color: String,
}

#[derive(Encodable, Decodable)]
pub struct GameServerBootData {
    pub protocol: u16,
    pub no_chat: Vec<i32>,
    pub special_users: IntMap<i32, SpecialUser>,
    pub tps: u32,
    pub maintenance: bool,
    pub secret_key2: String,
    pub token_expiry: u64,
}

impl Default for GameServerBootData {
    fn default() -> Self {
        Self {
            protocol: PROTOCOL_VERSION,
            no_chat: Vec::new(),
            special_users: IntMap::default(),
            tps: 30,
            maintenance: false,
            secret_key2: String::new(),
            token_expiry: 0,
        }
    }
}
