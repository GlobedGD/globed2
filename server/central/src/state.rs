use std::{
    collections::HashMap,
    net::IpAddr,
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};

use anyhow::anyhow;
use async_rate_limit::sliding_window::SlidingWindowRateLimiter;
use base64::{engine::general_purpose as b64e, Engine};
use hmac::{Hmac, Mac};
use sha2::Sha256;
use tokio::sync::{Mutex, RwLock, RwLockReadGuard, RwLockWriteGuard};

use crate::config::{ServerConfig, UserlistMode};
use blake2::{Blake2b, Digest};
use digest::consts::U32;
use totp_rs::{Algorithm, Secret, TOTP};

#[derive(Clone)]
pub struct ActiveChallenge {
    pub account_id: i32,
    pub value: String,
    pub started: Duration,
}

pub struct ServerStateData {
    pub config_path: PathBuf,
    pub config: ServerConfig,
    pub hmac: Hmac<Sha256>,
    pub active_challenges: HashMap<IpAddr, ActiveChallenge>,
    pub http_client: reqwest::Client,
    pub login_attempts: HashMap<IpAddr, Instant>,
    pub ratelimiter: Arc<Mutex<SlidingWindowRateLimiter>>,
}

impl ServerStateData {
    pub fn new(config_path: PathBuf, config: ServerConfig, secret_key: &str) -> Self {
        let skey_bytes = secret_key.as_bytes();
        let hmac = Hmac::<Sha256>::new_from_slice(skey_bytes).unwrap();

        let http_client = reqwest::ClientBuilder::new().user_agent("").build().unwrap();

        let api_rl = config.gd_api_ratelimit;
        let api_rl_period = config.gd_api_period;

        Self {
            config_path,
            config,
            hmac,
            active_challenges: HashMap::new(),
            http_client,
            login_attempts: HashMap::new(),
            ratelimiter: Arc::new(Mutex::new(SlidingWindowRateLimiter::new(
                Duration::from_secs(api_rl_period),
                api_rl,
            ))),
        }
    }

    // uses hmac-sha256 to derive an auth key from user's account ID and name
    pub fn generate_authkey(&self, account_id: i32, account_name: &str) -> Vec<u8> {
        let val = format!("{account_id}:{account_name}");

        let mut hmac: Hmac<Sha256> = self.hmac.clone();
        hmac.update(val.as_bytes());
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
    pub fn generate_token(&self, account_id: i32, account_name: &str) -> String {
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("whoops our clock went backwards")
            .as_secs();

        let data = format!("{account_id}.{account_name}.{timestamp}");
        let mut hmac = self.hmac.clone();
        hmac.update(data.as_bytes());
        let res = hmac.finalize();

        format!(
            "{}.{}",
            b64e::URL_SAFE_NO_PAD.encode(data),
            b64e::URL_SAFE_NO_PAD.encode(res.into_bytes())
        )
    }

    // verify the token and return the username if it's valid
    pub fn verify_token(&self, account_id: i32, token: &str) -> anyhow::Result<String> {
        let timestamp = SystemTime::now();

        let (claims, signature) = token.split_once('.').ok_or(anyhow!("malformed token"))?;

        let data_str = String::from_utf8(b64e::URL_SAFE_NO_PAD.decode(claims)?)?;
        let mut claims = data_str.split('.');

        let orig_id = claims.next().ok_or(anyhow!("malformed token"))?.parse::<i32>()?;
        let orig_name = claims.next().ok_or(anyhow!("malformed token"))?;
        let orig_ts = claims.next().ok_or(anyhow!("malformed token"))?;

        if orig_id != account_id {
            return Err(anyhow!("token validation failed"));
        }

        let elapsed = timestamp.duration_since(UNIX_EPOCH + Duration::from_secs(orig_ts.parse::<u64>()?))?;

        if elapsed > Duration::from_secs(self.config.token_expiry) {
            return Err(anyhow!("expired token, please reauthenticate"));
        }

        // verify the signature
        let mut hmac = self.hmac.clone();
        hmac.update(data_str.as_bytes());

        let signature = b64e::URL_SAFE_NO_PAD.decode(signature)?;

        hmac.verify_slice(&signature)?;

        Ok(orig_name.to_string())
    }

    pub fn is_ratelimited(&self, addr: &IpAddr) -> bool {
        match self.login_attempts.get(addr) {
            None => false,
            Some(last) => {
                let passed = Instant::now().duration_since(*last).as_secs();
                passed < self.config.challenge_ratelimit
            }
        }
    }

    pub fn record_login_attempt(&mut self, addr: &IpAddr) -> anyhow::Result<()> {
        if self.is_ratelimited(addr) {
            return Err(anyhow!("you are doing this too fast, please try again later"));
        }

        self.login_attempts.insert(*addr, Instant::now());
        Ok(())
    }

    pub fn should_block(&self, account_id: i32) -> bool {
        match self.config.userlist_mode {
            UserlistMode::None => false,
            UserlistMode::Blacklist => self.config.userlist.contains(&account_id),
            UserlistMode::Whitelist => !self.config.userlist.contains(&account_id),
        }
    }
}

// both roa::Context and RwLock have the methods read() and write()
// so doing Context<RwLock<..>> will break some things, hence we make a wrapper

#[derive(Clone)]
pub struct ServerState {
    pub inner: Arc<(RwLock<ServerStateData>, AtomicBool)>,
}

impl ServerState {
    pub fn new(ssd: ServerStateData) -> Self {
        let maintenance = ssd.config.maintenance;
        ServerState {
            inner: Arc::new((RwLock::new(ssd), AtomicBool::new(maintenance))),
        }
    }

    pub async fn state_read(&self) -> RwLockReadGuard<'_, ServerStateData> {
        self.inner.0.read().await
    }

    pub async fn state_write(&self) -> RwLockWriteGuard<'_, ServerStateData> {
        self.inner.0.write().await
    }

    pub fn maintenance(&self) -> bool {
        self.inner.1.load(Ordering::Relaxed)
    }

    pub fn set_maintenance(&self, state: bool) {
        self.inner.1.store(state, Ordering::Relaxed);
    }
}
