use std::{
    collections::HashMap,
    fs::{File, OpenOptions},
    path::Path,
};

use globed_shared::SpecialUser;
use rand::{distributions::Alphanumeric, Rng};
use serde::{Deserialize, Serialize};
use serde_json::{ser::PrettyFormatter, Serializer};

/* stinky serde defaults */

fn default_web_mountpoint() -> String {
    "/".to_string()
}

fn default_web_address() -> String {
    "0.0.0.0:41000".to_string()
}

fn default_use_gd_api() -> bool {
    false
}

fn default_gdapi() -> String {
    "http://www.boomlings.com/database/getGJComments21.php".to_string()
}

fn default_gdapi_ratelimit() -> usize {
    5
}

fn default_gdapi_period() -> u64 {
    5
}

fn default_game_servers() -> Vec<GameServerEntry> {
    Vec::new()
}

fn default_special_users() -> HashMap<i32, SpecialUser> {
    HashMap::new()
}

fn default_userlist_mode() -> UserlistMode {
    UserlistMode::None
}

fn default_userlist() -> Vec<i32> {
    Vec::new()
}

fn default_secret_key() -> String {
    let rand_string: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(32)
        .map(char::from)
        .collect();

    format!("Insecure-{rand_string}")
}

fn default_challenge_expiry() -> u32 {
    30
}

fn default_challenge_level() -> i32 {
    1
}

fn default_challenge_ratelimit() -> u64 {
    60
}

fn default_cloudflare_protection() -> bool {
    false
}

fn default_token_expiry() -> u64 {
    60 * 60 * 24
}

/* end stinky serde defaults */

#[derive(PartialEq, Debug, Default, Clone, Serialize, Deserialize)]
pub enum UserlistMode {
    #[serde(rename = "blacklist")]
    Blacklist,
    #[serde(rename = "whitelist")]
    Whitelist,
    #[default]
    #[serde(rename = "none")]
    None,
}

#[derive(Serialize, Deserialize, Default, Clone)]
pub struct GameServerEntry {
    pub id: String,
    pub name: String,
    pub address: String,
    pub region: String,
}

#[derive(Serialize, Deserialize, Default, Clone)]
pub struct ServerConfig {
    #[serde(default = "default_web_mountpoint")]
    pub web_mountpoint: String,
    #[serde(default = "default_web_address")]
    pub web_address: String,
    #[serde(default = "default_game_servers")]
    pub game_servers: Vec<GameServerEntry>,

    // special users and "special" users
    #[serde(default = "default_special_users")]
    pub special_users: HashMap<i32, SpecialUser>,
    #[serde(default = "default_userlist_mode")]
    pub userlist_mode: UserlistMode,
    #[serde(default = "default_userlist")]
    pub userlist: Vec<i32>,
    #[serde(default = "default_userlist")]
    pub no_chat_list: Vec<i32>,

    // security
    #[serde(default = "default_use_gd_api")]
    pub use_gd_api: bool,
    #[serde(default = "default_gdapi")]
    pub gd_api: String,
    #[serde(default = "default_gdapi_ratelimit")]
    pub gd_api_ratelimit: usize,
    #[serde(default = "default_gdapi_period")]
    pub gd_api_period: u64,
    #[serde(default = "default_secret_key")]
    pub secret_key: String,
    #[serde(default = "default_secret_key")]
    pub game_server_password: String,
    #[serde(default = "default_cloudflare_protection")]
    pub cloudflare_protection: bool,
    #[serde(default = "default_challenge_expiry")]
    pub challenge_expiry: u32,
    #[serde(default = "default_challenge_level")]
    pub challenge_level: i32,
    #[serde(default = "default_challenge_ratelimit")]
    pub challenge_ratelimit: u64,
    #[serde(default = "default_token_expiry")]
    pub token_expiry: u64,
}

impl ServerConfig {
    pub fn load(source: &Path) -> anyhow::Result<Self> {
        Ok(serde_json::from_reader(File::open(source)?)?)
    }

    pub fn save(&self, dest: &Path) -> anyhow::Result<()> {
        let writer = OpenOptions::new().write(true).create(true).open(dest)?;

        // i hate 2 spaces i hate 2 spaces i hate 2 spaces
        let mut serializer = Serializer::with_formatter(writer, PrettyFormatter::with_indent(b"    "));
        self.serialize(&mut serializer)?;

        Ok(())
    }

    pub fn reload_in_place(&mut self, source: &Path) -> anyhow::Result<()> {
        let conf = ServerConfig::load(source)?;
        self.clone_from(&conf);
        Ok(())
    }

    pub fn make_default() -> Self {
        // i'm okay thanks for asking
        Self {
            web_mountpoint: default_web_mountpoint(),
            web_address: default_web_address(),
            use_gd_api: default_use_gd_api(),
            gd_api: default_gdapi(),
            gd_api_ratelimit: default_gdapi_ratelimit(),
            gd_api_period: default_gdapi_period(),
            game_servers: default_game_servers(),
            special_users: default_special_users(),
            userlist_mode: default_userlist_mode(),
            userlist: default_userlist(),
            no_chat_list: default_userlist(),
            secret_key: default_secret_key(),
            game_server_password: default_secret_key(),
            challenge_expiry: default_challenge_expiry(),
            challenge_level: default_challenge_level(),
            challenge_ratelimit: default_challenge_ratelimit(),
            cloudflare_protection: default_cloudflare_protection(),
            token_expiry: default_token_expiry(),
        }
    }
}
