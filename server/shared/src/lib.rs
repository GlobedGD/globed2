#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::wildcard_imports
)]

use esp::{types::FastString, Decodable, DynamicSize, Encodable, StaticSize};
pub use globed_derive::{Decodable, DynamicSize, Encodable, StaticSize};
use rand::{distributions::Alphanumeric, Rng};

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
pub use reqwest;
pub use sha2;
pub use time;
// our reexports
pub use data::*;
pub use logger::*;
pub use token_issuer::TokenIssuer;
pub mod data;
pub mod logger;
pub mod token_issuer;

pub const PROTOCOL_VERSION: u16 = 4;
pub const SERVER_MAGIC: &[u8] = b"\xda\xeeglobed\xda\xee";
pub const SERVER_MAGIC_LEN: usize = SERVER_MAGIC.len();
/// amount of chars in an admin key (32)
pub const ADMIN_KEY_LENGTH: usize = 32;
/// maximum characters in a user's name (24). they can only be 15 chars max but we give headroom just in case
pub const MAX_NAME_SIZE: usize = 24;
pub const VIOLATION_REASON_LENGTH: usize = 128;

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
