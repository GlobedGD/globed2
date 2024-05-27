use super::*;
use esp::FastString;
use serde::{Deserialize, Serialize};

#[derive(Encodable, Decodable, Clone)]
pub struct GameServerBootData {
    pub protocol: u16,
    pub tps: u32,
    pub maintenance: bool,
    pub secret_key2: String,
    pub token_expiry: u64,
    pub status_print_interval: u64,
    pub admin_key: FastString,
    pub whitelist: bool,
    pub admin_webhook_url: String,
    pub chat_burst_limit: u32,
    pub chat_burst_interval: u32,
    pub roles: Vec<ServerRole>,
}

impl Default for GameServerBootData {
    fn default() -> Self {
        #[cfg(debug_assertions)]
        let status_print_interval = 60 * 15; // 15 min
        #[cfg(not(debug_assertions))]
        let status_print_interval = 7200; // 2 hours

        Self {
            protocol: PROTOCOL_VERSION,
            tps: 30,
            maintenance: false,
            secret_key2: String::new(),
            token_expiry: 0,
            status_print_interval,
            admin_key: generate_alphanum_string(ADMIN_KEY_LENGTH).into(),
            whitelist: false,
            admin_webhook_url: String::new(),
            chat_burst_limit: 0,
            chat_burst_interval: 0,
            roles: Vec::new(),
        }
    }
}

#[derive(Encodable, Decodable, Serialize, Deserialize, DynamicSize, Clone, Default)]
pub struct UserEntry {
    pub account_id: i32,
    pub user_name: Option<String>,
    pub name_color: Option<String>,
    pub user_roles: Vec<String>,
    pub is_banned: bool,
    pub is_muted: bool,
    pub is_whitelisted: bool,
    pub admin_password: Option<String>,
    pub violation_reason: Option<String>,
    pub violation_expiry: Option<i64>, // seconds since unix epoch
}

impl UserEntry {
    pub fn new(account_id: i32) -> Self {
        Self {
            account_id,
            ..Default::default()
        }
    }
}

#[derive(Serialize, Deserialize, Encodable, Decodable, DynamicSize, Clone, Default)]
#[allow(clippy::struct_excessive_bools)]
pub struct ServerRole {
    pub id: String,
    pub priority: i32,
    #[serde(default)]
    pub badge_icon: String,
    #[serde(default)]
    pub name_color: String,
    #[serde(default)]
    pub chat_color: String,

    // permissions
    #[serde(default)]
    pub notices: bool,
    #[serde(default)]
    pub notices_to_everyone: bool,
    #[serde(default)]
    pub kick: bool,
    #[serde(default)]
    pub kick_everyone: bool,
    #[serde(default)]
    pub mute: bool,
    #[serde(default)]
    pub ban: bool,
    #[serde(default)]
    pub edit_role: bool,
    #[serde(default)]
    pub admin: bool,
}
