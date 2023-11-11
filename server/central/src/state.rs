use std::{
    collections::HashMap,
    net::IpAddr,
    path::PathBuf,
    sync::Arc,
    time::{Duration, SystemTime, UNIX_EPOCH},
};

use anyhow::anyhow;
use base64::{engine::general_purpose as b64e, Engine};
use hmac::{Hmac, Mac};
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
    pub token_expiry: Duration,
}

impl ServerStateData {
    pub fn new(
        config_path: PathBuf,
        config: ServerConfig,
        secret_key: String,
        token_expiry: Duration,
    ) -> Self {
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
            token_expiry,
        }
    }

    // uses hmac-sha256 to derive an auth key from user's account ID
    pub fn generate_authkey(&self, account_id: &str) -> Vec<u8> {
        let mut hmac: Hmac<Sha256> = self.hmac.clone();
        hmac.update(account_id.as_bytes());
        let res = hmac.finalize();
        res.into_bytes().to_vec()
    }

    pub fn verify_totp(&self, key: &[u8], code: &str) -> bool {
        let mut hasher = Blake2b::<U32>::new();

        hasher.update(key);
        let output = hasher.finalize();

        let secret = Secret::Raw(output.to_vec()).to_bytes().unwrap();

        let totp = TOTP::new_unchecked(Algorithm::SHA256, 6, 1, 30, secret);
        totp.check_current(code).unwrap_or(false)
    }

    pub fn verify_challenge(&self, orig_value: &str, answer: &str) -> bool {
        self.verify_totp(orig_value.as_bytes(), answer)
    }

    // generate a token, similar to jwt but more efficient and lightweight
    pub fn generate_token(&self, account_id: &str) -> String {
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("whoops our clock went backwards")
            .as_secs();

        let data = format!("{}.{}", account_id, timestamp);
        let mut hmac = self.hmac.clone();
        hmac.update(data.as_bytes());
        let res = hmac.finalize();

        format!(
            "{}.{}",
            b64e::URL_SAFE_NO_PAD.encode(data),
            b64e::URL_SAFE_NO_PAD.encode(res.into_bytes())
        )
    }

    pub fn verify_token(&self, account_id: &str, token: &str) -> anyhow::Result<()> {
        let timestamp = SystemTime::now();

        let (claims, signature) = token.split_once('.').ok_or(anyhow!("malformed token"))?;

        let data_str = String::from_utf8(b64e::URL_SAFE_NO_PAD.decode(claims)?)?;
        let (orig_id, orig_ts) = data_str.split_once('.').ok_or(anyhow!("malformed token"))?;

        if orig_id != account_id {
            return Err(anyhow!("token validation failed"));
        }

        let elapsed =
            timestamp.duration_since(UNIX_EPOCH + Duration::from_secs(orig_ts.parse::<u64>()?))?;

        if elapsed > self.token_expiry {
            return Err(anyhow!("expired token, please reauthenticate"));
        }

        // verify the signature
        let mut hmac = self.hmac.clone();
        hmac.update(data_str.as_bytes());

        let signature = b64e::URL_SAFE_NO_PAD.decode(signature)?;

        hmac.verify_slice(&signature)?;

        Ok(())
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
