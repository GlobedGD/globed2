use std::time::UNIX_EPOCH;

use super::*;
use argon2::{
    Argon2,
    password_hash::{PasswordHash, PasswordHasher, PasswordVerifier, SaltString, rand_core::OsRng},
};
use esp::{FastString, InlineString};
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
    pub rate_suggestion_webhook_url: String,
    pub featured_webhook_url: String,
    pub room_webhook_url: String,
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
            protocol: MAX_SUPPORTED_PROTOCOL,
            tps: 30,
            maintenance: false,
            secret_key2: String::new(),
            token_expiry: 0,
            status_print_interval,
            admin_key: generate_alphanum_string(ADMIN_KEY_LENGTH).into(),
            whitelist: false,
            admin_webhook_url: String::new(),
            rate_suggestion_webhook_url: String::new(),
            featured_webhook_url: String::new(),
            room_webhook_url: String::new(),
            chat_burst_limit: 0,
            chat_burst_interval: 0,
            roles: Vec::new(),
        }
    }
}

pub fn generate_argon2_hash(password: &str) -> String {
    let salt = SaltString::generate(&mut OsRng);

    Argon2::default()
        .hash_password(password.as_bytes(), &salt)
        .map(|x| x.to_string())
        .unwrap()
}

#[derive(Encodable, Decodable, Serialize, Deserialize, DynamicSize, Clone, Default)]
pub struct ServerUserEntry {
    pub account_id: i32,
    pub user_name: Option<String>,
    pub name_color: Option<String>,
    pub user_roles: Vec<String>,
    pub is_whitelisted: bool,
    pub admin_password: Option<String>,
    pub admin_password_hash: Option<String>,
    pub active_mute: Option<i64>,
    pub active_ban: Option<i64>,
}

// this is pure laziness i was just too lazy to get derive macros n shit into the central server
#[derive(Decodable, Clone)]
pub struct UserLoginData {
    pub account_id: i32,
    pub name: String,
}

// same here
#[derive(Encodable, Decodable, Clone)]
pub struct UserLoginResponse {
    pub user_entry: ServerUserEntry,
    pub ban: Option<UserPunishment>,
    pub link_code: u32,
}

impl ServerUserEntry {
    pub fn new(account_id: i32) -> Self {
        Self {
            account_id,
            ..Default::default()
        }
    }

    pub fn to_user_entry(self, active_ban: Option<UserPunishment>, active_mute: Option<UserPunishment>) -> UserEntry {
        UserEntry {
            account_id: self.account_id,
            user_name: self.user_name,
            name_color: self.name_color,
            user_roles: self.user_roles,
            is_whitelisted: self.is_whitelisted,
            active_ban,
            active_mute,
        }
    }

    pub fn verify_password(&self, password: &str) -> Result<bool, argon2::password_hash::Error> {
        match self.admin_password_hash.as_ref() {
            Some(x) => {
                let parsed_hash = PasswordHash::new(x)?;

                Ok(Argon2::default().verify_password(password.as_bytes(), &parsed_hash).is_ok())
            }
            None => Ok(false),
        }
    }
}

#[derive(Clone, Copy, Encodable, Decodable, DynamicSize, StaticSize, Serialize, Deserialize)]
#[repr(u8)]
#[dynamic_size(as_static)]
pub enum PunishmentType {
    Ban = 0,
    Mute = 1,
}

#[derive(Clone, Encodable, Decodable, DynamicSize, Serialize, Deserialize)]
pub struct UserPunishment {
    pub id: i64,
    pub account_id: i32,
    pub r#type: PunishmentType,
    pub reason: String,
    pub expires_at: i64,
    pub issued_at: Option<i64>,
    pub issued_by: Option<i32>,
}

impl UserPunishment {
    pub fn expired(&self) -> bool {
        self.expires_at != 0 && (self.expires_at as u64) < UNIX_EPOCH.elapsed().unwrap().as_secs()
    }
}

#[derive(Encodable, Decodable, Serialize, Deserialize, DynamicSize, Clone, Default)]
pub struct UserEntry {
    pub account_id: i32,
    pub user_name: Option<String>,
    pub name_color: Option<String>,
    pub user_roles: Vec<String>,
    pub is_whitelisted: bool,
    pub active_ban: Option<UserPunishment>,
    pub active_mute: Option<UserPunishment>,
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
    pub edit_featured_levels: bool,
    #[serde(default)]
    pub admin: bool,
}

/* Admin actions */

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminUpdateUsernameAction {
    pub account_id: i32,
    pub username: InlineString<MAX_NAME_SIZE>,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminSetNameColorAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub color: FastString,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminSetUserRolesAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub roles: Vec<String>,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminPunishUserAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub is_ban: bool,
    pub reason: FastString,
    pub expires_at: u64,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminRemovePunishmentAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub is_ban: bool,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminWhitelistAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub state: bool,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminSetAdminPasswordAction {
    pub account_id: i32,
    pub new_password: FastString,
}

#[derive(Decodable, Encodable, DynamicSize)]
pub struct AdminEditPunishmentAction {
    pub issued_by: i32,
    pub account_id: i32,
    pub is_ban: bool,
    pub reason: FastString,
    pub expires_at: u64,
}
