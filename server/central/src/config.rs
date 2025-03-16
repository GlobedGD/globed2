use std::{
    fs::{File, OpenOptions},
    path::Path,
};

use globed_shared::{
    ADMIN_KEY_LENGTH, DEFAULT_GAME_SERVER_PORT, Decodable, Encodable, ServerRole,
    anyhow::{self, anyhow},
    esp::{self, Decodable, Encodable},
    generate_alphanum_string,
};
use json_comments::StripComments;
use serde::{Deserialize, Serialize};
use serde_json::{Serializer, ser::PrettyFormatter};

/* stinky serde defaults */

const fn default_false() -> bool {
    false
}

#[allow(unused)]
const fn default_true() -> bool {
    true
}

fn default_string() -> String {
    String::new()
}

fn default_web_mountpoint() -> String {
    "/".to_string()
}

fn default_admin_key() -> String {
    generate_alphanum_string(ADMIN_KEY_LENGTH)
}

const fn default_gd_api_account() -> i32 {
    0
}

fn default_gd_api_url() -> String {
    "https://www.boomlings.com/database".to_owned()
}

const fn default_refresh_interval() -> u64 {
    3000 // 3 seconds
}

fn default_game_servers() -> Vec<GameServerEntry> {
    vec![GameServerEntry {
        id: "example-server-you-can-delete-it".to_owned(),
        name: "Server name".to_owned(),
        address: format!("127.0.0.1:{DEFAULT_GAME_SERVER_PORT}"),
        region: "the nether".to_owned(),
    }]
}

const fn default_status_print_interval() -> u64 {
    7200 // 2 hours
}

const fn default_userlist_mode() -> UserlistMode {
    UserlistMode::None
}

const fn default_tps() -> u32 {
    30
}

const fn default_chat_burst_limit() -> u32 {
    2
}

const fn default_chat_burst_interval() -> u32 {
    3000
}

fn default_roles() -> Vec<ServerRole> {
    vec![
        ServerRole {
            id: "admin".to_owned(),
            priority: 10000,
            badge_icon: "role-admin.png".to_owned(),
            name_color: "#e91e63 > e91ec7".to_owned(),
            admin: true,
            ..Default::default()
        },
        ServerRole {
            id: "mod".to_owned(),
            priority: 1000,
            badge_icon: "role-mod.png".to_owned(),
            name_color: "#0fefc3".to_owned(),
            notices: true,
            kick: true,
            mute: true,
            ban: true,
            edit_role: true,
            ..Default::default()
        },
    ]
}

fn default_secret_key() -> String {
    let rand_string = generate_alphanum_string(32);

    format!("Insecure-{rand_string}")
}

const fn default_challenge_expiry() -> u32 {
    30
}

const fn default_token_expiry() -> u64 {
    60 * 60 * 24
}

/* end stinky serde defaults */

#[derive(PartialEq, Eq, Debug, Default, Clone, Serialize, Deserialize)]
pub enum UserlistMode {
    #[serde(rename = "blacklist")]
    Blacklist,
    #[serde(rename = "whitelist")]
    Whitelist,
    #[default]
    #[serde(rename = "none")]
    None, // same as blacklist
}

#[derive(Serialize, Deserialize, Encodable, Decodable, Default, Clone)]
pub struct GameServerEntry {
    pub id: String,
    pub name: String,
    pub address: String,
    pub region: String,
}

#[derive(Serialize, Deserialize, Clone)]
#[allow(clippy::struct_excessive_bools)]
pub struct ServerConfig {
    #[serde(default = "default_web_mountpoint")]
    pub web_mountpoint: String,
    #[serde(default = "default_game_servers")]
    pub game_servers: Vec<GameServerEntry>,
    #[serde(default = "default_false")]
    pub maintenance: bool,
    #[serde(default = "default_status_print_interval")]
    pub status_print_interval: u64,

    // special users and "special" users
    #[serde(default = "default_userlist_mode")]
    pub userlist_mode: UserlistMode,

    // game stuff
    #[serde(default = "default_tps")]
    pub tps: u32,

    // webhooks
    #[serde(default = "default_string")]
    pub admin_webhook_url: String,
    #[serde(default = "default_string")]
    pub rate_suggestion_webhook_url: String,
    #[serde(default = "default_string")]
    pub featured_webhook_url: String,
    #[serde(default = "default_string")]
    pub featured_webhook_message: String,
    #[serde(default = "default_string")]
    pub room_webhook_url: String,

    // chat limits
    #[serde(default = "default_chat_burst_limit")]
    pub chat_burst_limit: u32,
    #[serde(default = "default_chat_burst_interval")]
    pub chat_burst_interval: u32,

    // roles
    #[serde(default = "default_roles")]
    pub roles: Vec<ServerRole>,

    // security
    #[serde(default = "default_admin_key")]
    pub admin_key: String,
    #[serde(default = "default_false")]
    pub use_gd_api: bool,
    #[serde(default = "default_gd_api_account")]
    pub gd_api_account: i32,
    #[serde(default = "default_string")]
    pub gd_api_gjp: String,
    #[serde(default = "default_gd_api_url")]
    pub gd_api_url: String,
    #[serde(default = "default_false")]
    pub skip_name_check: bool,
    #[serde(default = "default_refresh_interval")]
    pub refresh_interval: u64,
    #[serde(default = "default_secret_key")]
    pub secret_key: String,
    #[serde(default = "default_secret_key")]
    pub secret_key2: String,
    #[serde(default = "default_secret_key")]
    pub game_server_password: String,
    #[serde(default = "default_false")]
    pub cloudflare_protection: bool,
    #[serde(default = "default_challenge_expiry")]
    pub challenge_expiry: u32,
    #[serde(default = "default_token_expiry")]
    pub token_expiry: u64,
}

impl ServerConfig {
    pub fn load(source: &Path) -> anyhow::Result<Self> {
        let file = File::open(source)?;
        let stripped = StripComments::new(file);

        Ok(serde_json::from_reader(stripped)?)
    }

    pub fn save(&self, dest: &Path) -> anyhow::Result<()> {
        let writer = OpenOptions::new().write(true).create(true).truncate(true).open(dest)?;

        // i hate 2 spaces i hate 2 spaces i hate 2 spaces
        let formatter = PrettyFormatter::with_indent(b"    ");
        let mut serializer = Serializer::with_formatter(writer, formatter);
        self.serialize(&mut serializer)?;

        Ok(())
    }

    pub fn reload_in_place(&mut self, source: &Path) -> anyhow::Result<()> {
        let conf = Self::load(source)?;

        // Do validation
        if conf.admin_key.len() > ADMIN_KEY_LENGTH {
            return Err(anyhow!("Invalid admin key size, must be {ADMIN_KEY_LENGTH} characters or less"));
        }

        self.clone_from(&conf);
        Ok(())
    }
}

impl Default for ServerConfig {
    fn default() -> Self {
        // i'm just so cool like that
        serde_json::from_str("{}").unwrap()
    }
}
