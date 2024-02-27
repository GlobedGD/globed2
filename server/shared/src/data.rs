use super::*;

#[derive(Encodable, Decodable, Clone)]
pub struct GameServerBootData {
    pub protocol: u16,
    pub tps: u32,
    pub maintenance: bool,
    pub secret_key2: String,
    pub token_expiry: u64,
    pub status_print_interval: u64,
    pub admin_key: FastString<ADMIN_KEY_LENGTH>,
    pub whitelist: bool,
    pub admin_webhook_url: String,
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
            admin_key: generate_alphanum_string(ADMIN_KEY_LENGTH)
                .try_into()
                .expect("failed to convert admin key String into FastString"),
            whitelist: false,
            admin_webhook_url: String::new(),
        }
    }
}

#[derive(Encodable, Decodable, StaticSize, DynamicSize, Clone, Default)]
pub struct UserEntry {
    pub account_id: i32,
    pub user_name: Option<FastString<MAX_NAME_SIZE>>,
    pub name_color: Option<FastString<7>>,
    pub user_role: i32,
    pub is_banned: bool,
    pub is_muted: bool,
    pub is_whitelisted: bool,
    pub admin_password: Option<FastString<ADMIN_KEY_LENGTH>>,
    pub violation_reason: Option<FastString<VIOLATION_REASON_LENGTH>>,
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
