use std::{
    fs::{File, OpenOptions},
    path::Path,
};

use globed_shared::{
    anyhow::{self, anyhow},
    esp::{self, Decodable, Encodable},
    generate_alphanum_string, Decodable, Encodable, ADMIN_KEY_LENGTH,
};
use json_comments::StripComments;
use serde::{Deserialize, Serialize};
use serde_json::{ser::PrettyFormatter, Serializer};

/* stinky serde defaults */

fn default_web_mountpoint() -> String {
    "/".to_string()
}

fn default_admin_key() -> String {
    generate_alphanum_string(ADMIN_KEY_LENGTH)
}

const fn default_use_gd_api() -> bool {
    false
}

const fn default_gd_api_account() -> i32 {
    0
}

fn default_gd_api_gjp() -> String {
    String::new()
}

fn default_gd_api_url() -> String {
    "https://www.boomlings.com/database".to_owned()
}

fn default_game_servers() -> Vec<GameServerEntry> {
    vec![GameServerEntry {
        id: "example-server-you-can-delete-it".to_owned(),
        name: "Server name".to_owned(),
        address: "127.0.0.1:41001".to_owned(),
        region: "the nether".to_owned(),
    }]
}

const fn default_maintenance() -> bool {
    false
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

fn default_admin_webhook_url() -> String {
    String::new()
}

fn default_secret_key() -> String {
    let rand_string = generate_alphanum_string(32);

    format!("Insecure-{rand_string}")
}

const fn default_challenge_expiry() -> u32 {
    30
}

const fn default_cloudflare_protection() -> bool {
    false
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
pub struct ServerConfig {
    #[serde(default = "default_web_mountpoint")]
    pub web_mountpoint: String,
    #[serde(default = "default_game_servers")]
    pub game_servers: Vec<GameServerEntry>,
    #[serde(default = "default_maintenance")]
    pub maintenance: bool,
    #[serde(default = "default_status_print_interval")]
    pub status_print_interval: u64,

    // special users and "special" users
    #[serde(default = "default_userlist_mode")]
    pub userlist_mode: UserlistMode,

    // game stuff
    #[serde(default = "default_tps")]
    pub tps: u32,

    #[serde(default = "default_admin_webhook_url")]
    pub admin_webhook_url: String,

    // security
    #[serde(default = "default_admin_key")]
    pub admin_key: String,
    #[serde(default = "default_use_gd_api")]
    pub use_gd_api: bool,
    #[serde(default = "default_gd_api_account")]
    pub gd_api_account: i32,
    #[serde(default = "default_gd_api_gjp")]
    pub gd_api_gjp: String,
    #[serde(default = "default_gd_api_url")]
    pub gd_api_url: String,
    #[serde(default = "default_secret_key")]
    pub secret_key: String,
    #[serde(default = "default_secret_key")]
    pub secret_key2: String,
    #[serde(default = "default_secret_key")]
    pub game_server_password: String,
    #[serde(default = "default_cloudflare_protection")]
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
            return Err(anyhow!(
                "Invalid admin key size, must be {ADMIN_KEY_LENGTH} characters or less"
            ));
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
