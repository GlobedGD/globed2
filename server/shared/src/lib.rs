#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc
)]

use esp::{types::FastString, Decodable, Encodable};
pub use globed_derive::{Decodable, Encodable};
use rand::{distributions::Alphanumeric, Rng};
use serde::{Deserialize, Serialize};

// import reexports
pub use nohash_hasher::{IntMap, IntSet};
pub use parking_lot::{Mutex as SyncMutex, MutexGuard as SyncMutexGuard};
// module reexports
pub use anyhow;
pub use base64;
pub use colored;
pub use crypto_box;
pub use esp;
pub use hmac;
pub use parking_lot;
pub use rand;
pub use sha2;
pub use time;
// our reexports
pub use logger::*;
pub use token_issuer::TokenIssuer;
pub mod logger;
pub mod token_issuer;

pub const PROTOCOL_VERSION: u16 = 2;
pub const SERVER_MAGIC: &[u8] = b"\xda\xeeglobed\xda\xee";
pub const SERVER_MAGIC_LEN: usize = SERVER_MAGIC.len();
/// amount of chars in an admin key (16)
pub const ADMIN_KEY_LENGTH: usize = 16;

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
    pub status_print_interval: u64,
    pub admin_key: FastString<ADMIN_KEY_LENGTH>,
}

pub fn generate_alphanum_string(n: usize) -> String {
    rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(n)
        .map(char::from)
        .collect()
}

pub fn get_log_level(env_var: &str) -> Option<LogLevelFilter> {
    std::env::var(env_var).map_or_else(
        |_| {
            Some(if cfg!(debug_assertions) {
                LogLevelFilter::Trace
            } else {
                LogLevelFilter::Info
            })
        },
        |level| match &*level.to_lowercase() {
            "trace" => Some(LogLevelFilter::Trace),
            "debug" => Some(LogLevelFilter::Debug),
            "info" => Some(LogLevelFilter::Info),
            "warn" => Some(LogLevelFilter::Warn),
            "error" => Some(LogLevelFilter::Error),
            "off" => Some(LogLevelFilter::Off),
            _ => None,
        },
    )
}

impl Default for GameServerBootData {
    fn default() -> Self {
        #[cfg(debug_assertions)]
        let status_print_interval = 60 * 15; // 15 min
        #[cfg(not(debug_assertions))]
        let status_print_interval = 7200; // 2 hours

        Self {
            protocol: PROTOCOL_VERSION,
            no_chat: Vec::new(),
            special_users: IntMap::default(),
            tps: 30,
            maintenance: false,
            secret_key2: String::new(),
            token_expiry: 0,
            status_print_interval,
            admin_key: generate_alphanum_string(ADMIN_KEY_LENGTH)
                .try_into()
                .expect("failed to convert admin key String into FastString"),
        }
    }
}
