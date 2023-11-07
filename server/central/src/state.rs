use std::{
    collections::HashMap,
    net::{IpAddr, SocketAddr},
    path::PathBuf,
    sync::Arc,
    time::Duration,
};

use hmac::{Hmac, Mac};
use log::warn;
use sha2::Sha256;
use tokio::sync::{RwLock, RwLockReadGuard, RwLockWriteGuard};

use crate::config::ServerConfig;
use blake2::{Blake2b, Digest};
use digest::consts::U32;
use totp_rs::{Algorithm, Secret, TOTP};

#[derive(Clone)]
pub struct ActiveChallenge {
    pub value: String,
    pub started: Duration,
    pub initiator: IpAddr,
}

pub struct ServerStateData {
    pub config_path: PathBuf,
    pub config: ServerConfig,
    pub hmac: Hmac<Sha256>,
    pub active_challenges: HashMap<i32, ActiveChallenge>,
    pub http_client: reqwest::Client,
}

impl ServerStateData {
    pub fn new(config_path: PathBuf, config: ServerConfig, secret_key: String) -> Self {
        let skey_bytes = secret_key.as_bytes();
        let hmac_obj = Hmac::<Sha256>::new_from_slice(skey_bytes).unwrap();

        let http_client = reqwest::ClientBuilder::new()
            .user_agent("")
            .build()
            .unwrap();

        Self {
            config_path,
            config,
            hmac: hmac_obj,
            active_challenges: HashMap::new(),
            http_client,
        }
    }

    // uses hmac-sha256 to derive an auth key from user's account ID
    pub fn generate_authkey(&self, account_id: &str) -> Vec<u8> {
        let mut hmac = self.hmac.clone();
        hmac.update(account_id.as_bytes());
        let res = hmac.finalize();
        res.into_bytes().to_vec()
    }

    pub fn verify_totp(&self, key: &[u8], code: &str) -> bool {
        let mut hasher = Blake2b::<U32>::new();
        hasher.update(key);
        let output = hasher.finalize();

        let secret = Secret::Raw(output.to_vec()).to_bytes().unwrap();

        match TOTP::new(Algorithm::SHA256, 6, 1, 30, secret) {
            Ok(totp) => totp.check_current(code).unwrap_or(false),
            Err(err) => {
                warn!("Failed to create TOTP instance: {}", err.to_string());
                false
            }
        }
    }

    pub fn verify_challenge(&self, orig_value: &str, answer: &str) -> bool {
        self.verify_totp(orig_value.as_bytes(), answer)
    }
}

// both roa::Context and RwLock have methods like read() and write()
// so doing Context<RwLock<..>> will break some things, hence we make a wrapper

#[derive(Clone)]
pub struct ServerState {
    pub inner: Arc<RwLock<ServerStateData>>,
}

impl ServerState {
    pub async fn state_read(&self) -> RwLockReadGuard<'_, ServerStateData> {
        self.inner.read().await
    }

    pub async fn state_write(&self) -> RwLockWriteGuard<'_, ServerStateData> {
        self.inner.write().await
    }
}
