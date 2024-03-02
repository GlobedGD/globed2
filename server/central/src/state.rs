use std::{
    collections::HashMap,
    net::IpAddr,
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
    time::Duration,
};

use globed_shared::{
    anyhow,
    hmac::{Hmac, Mac},
    sha2::Sha256,
    TokenIssuer,
};
use tokio::sync::{RwLock, RwLockReadGuard, RwLockWriteGuard};

use crate::{
    config::{ServerConfig, UserlistMode},
    db::GlobedDb,
    verifier::AccountVerifier,
};
use blake2::{Blake2b, Digest};
use digest::consts::U32;
use totp_rs::{Algorithm, Secret, TOTP};

#[derive(Clone)]
pub struct ActiveChallenge {
    pub account_id: i32,
    pub user_id: i32,
    pub name: String,
    pub value: String,
    pub started: Duration,
}

pub struct ServerStateData {
    pub config_path: PathBuf,
    pub config: ServerConfig,
    pub hmac: Hmac<Sha256>,
    pub token_issuer: TokenIssuer,
    pub active_challenges: HashMap<IpAddr, ActiveChallenge>,
}

impl ServerStateData {
    pub fn new(config_path: PathBuf, config: ServerConfig, secret_key: &str, token_secret_key: &str) -> Self {
        let skey_bytes = secret_key.as_bytes();
        let hmac = Hmac::<Sha256>::new_from_slice(skey_bytes).unwrap();

        let token_expiry = Duration::from_secs(config.token_expiry);

        Self {
            config_path,
            config,
            hmac,
            token_issuer: TokenIssuer::new(token_secret_key, token_expiry),
            active_challenges: HashMap::new(),
        }
    }

    // uses hmac-sha256 to derive an auth key from user's account ID and name
    pub fn generate_authkey(&self, account_id: i32, user_id: i32, account_name: &str) -> Vec<u8> {
        let val = format!("{account_id}:{user_id}:{account_name}");

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

        let totp = TOTP::new_unchecked(Algorithm::SHA256, 6, 2, 30, secret);
        totp.check_current(code).unwrap_or(false)
    }

    pub fn verify_challenge(&self, orig_value: &str, answer: &str) -> bool {
        self.verify_totp(orig_value.as_bytes(), answer)
    }

    pub async fn is_banned(&self, db: &GlobedDb, account_id: i32) -> anyhow::Result<Option<String>> {
        if self.config.userlist_mode == UserlistMode::Whitelist {
            return Ok(None);
        }

        let user = db.get_user(account_id).await?;
        if let Some(user) = user {
            if user.is_banned {
                Ok(user.violation_reason.map(|x| x.try_to_string()))
            } else {
                Ok(None)
            }
        } else {
            Ok(None)
        }
    }
}

// both roa::Context and RwLock have the methods read() and write()
// so doing Context<RwLock<..>> will break some things, hence we make a wrapper

pub struct InnerServerState {
    pub data: RwLock<ServerStateData>,
    pub maintenance: AtomicBool,
    pub verifier: AccountVerifier,
}

impl InnerServerState {
    pub fn new(ssd: ServerStateData, maintenance: bool) -> Self {
        let gd_api_account = ssd.config.gd_api_account;
        let gd_api_gjp = ssd.config.gd_api_gjp.clone();
        let base_api_url = ssd.config.gd_api_url.clone();
        let use_gd_api = ssd.config.use_gd_api;

        Self {
            data: RwLock::new(ssd),
            maintenance: AtomicBool::new(maintenance),
            verifier: AccountVerifier::new(gd_api_account, gd_api_gjp, base_api_url, use_gd_api),
        }
    }

    pub async fn state_read(&self) -> RwLockReadGuard<'_, ServerStateData> {
        self.data.read().await
    }

    pub async fn state_write(&self) -> RwLockWriteGuard<'_, ServerStateData> {
        self.data.write().await
    }
}

#[derive(Clone)]
pub struct ServerState {
    pub inner: Arc<InnerServerState>,
}

impl ServerState {
    pub fn new(ssd: ServerStateData) -> Self {
        let maintenance = ssd.config.maintenance;
        let inner = InnerServerState::new(ssd, maintenance);
        Self { inner: Arc::new(inner) }
    }

    pub async fn state_read(&self) -> RwLockReadGuard<'_, ServerStateData> {
        self.inner.state_read().await
    }

    pub async fn state_write(&self) -> RwLockWriteGuard<'_, ServerStateData> {
        self.inner.state_write().await
    }

    pub fn maintenance(&self) -> bool {
        self.inner.maintenance.load(Ordering::Relaxed)
    }

    pub fn set_maintenance(&self, state: bool) {
        self.inner.maintenance.store(state, Ordering::Relaxed);
    }

    pub fn get_verifier(&self) -> &AccountVerifier {
        &self.inner.verifier
    }
}
