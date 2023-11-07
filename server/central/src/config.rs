use std::{
    fs::{File, OpenOptions},
    path::Path,
};

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

fn default_secret_key() -> String {
    let rand_string: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(32)
        .map(char::from)
        .collect();

    format!("Change-Me-Please-Insecure-{}", rand_string)
}

fn default_challenge_expiry() -> u32 {
    60u32
}

fn default_challenge_level() -> i32 {
    1
}

/* end stinky serde defaults */

#[derive(Serialize, Deserialize, Default)]
pub struct ServerConfig {
    #[serde(default = "default_web_mountpoint")]
    pub web_mountpoint: String,
    #[serde(default = "default_web_address")]
    pub web_address: String,

    // security
    #[serde(default = "default_secret_key")]
    pub secret_key: String,
    #[serde(default = "default_challenge_expiry")]
    pub challenge_expiry: u32,
    #[serde(default = "default_challenge_level")]
    pub challenge_level: i32,
}

impl ServerConfig {
    pub fn load(source: &Path) -> anyhow::Result<Self> {
        Ok(serde_json::from_reader(File::open(source)?)?)
    }

    pub fn save(&self, dest: &Path) -> anyhow::Result<()> {
        let writer = OpenOptions::new().write(true).create(true).open(dest)?;

        // i hate 2 spaces i hate 2 spaces i hate 2 spaces
        let mut serializer =
            Serializer::with_formatter(writer, PrettyFormatter::with_indent(b"    "));
        self.serialize(&mut serializer)?;

        Ok(())
    }

    pub fn reload_in_place(&mut self, source: &Path) -> anyhow::Result<()> {
        let conf = ServerConfig::load(source)?;
        ServerConfig::copy_to(&conf, self);
        Ok(())
    }

    pub fn make_default() -> Self {
        Self {
            web_mountpoint: default_web_mountpoint(),
            web_address: default_web_address(),
            secret_key: default_secret_key(),
            challenge_expiry: default_challenge_expiry(),
            challenge_level: default_challenge_level(),
        }
    }

    fn copy_to(source: &ServerConfig, dest: &mut ServerConfig) {
        dest.web_mountpoint = source.web_mountpoint.clone();
        dest.web_address = source.web_address.clone();
        dest.secret_key = source.secret_key.clone();
        dest.challenge_expiry = source.challenge_expiry;
        dest.challenge_level = source.challenge_level;
    }
}
