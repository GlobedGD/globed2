use std::{net::IpAddr, time::SystemTime};

use digest::KeyInit;
use rocket::{post, State};
use serde::Deserialize;

use globed_shared::{
    anyhow::{self, anyhow},
    base64::{engine::general_purpose as b64e, Engine as _},
    crypto_box::aead::Aead,
    logger::*,
    MIN_CLIENT_VERSION,
};

use super::*;
use crate::{config::UserlistMode, ip_blocker::IpBlocker, state::ActiveChallenge};

#[derive(Debug, Deserialize)]
pub struct AccountData {
    account_id: i32,
    user_id: i32,
    username: String,
}

#[derive(Debug, Deserialize)]
pub struct TotpLoginData {
    account_data: AccountData,
    authkey: String,
}

#[derive(Debug, Deserialize)]
pub struct ChallengeFinishData {
    account_data: AccountData,
    answer: String,
    trust_token: Option<String>,
}

fn check_ip(ip: IpAddr, cfip: &CloudflareIPGuard, cloudflare: bool) -> WebResult<IpAddr> {
    let user_ip: anyhow::Result<IpAddr> = if cloudflare && !cfg!(debug_assertions) {
        // verify if the actual peer is cloudflare
        if !IpBlocker::instance().is_allowed(&ip) {
            warn!("blocking unknown non-cloudflare address: {}", ip);
            unauthorized!("access is denied from this IP address");
        }

        cfip.0.ok_or(anyhow!("failed to parse the IP header from Cloudflare"))
    } else {
        Ok(cfip.0.unwrap_or(ip))
    };

    match user_ip {
        Ok(x) => Ok(x),
        Err(err) => bad_request!(&err.to_string()),
    }
}

fn trim_name(data: &mut AccountData) {
    let trimmed = data.username.trim_end();
    trimmed.to_owned().clone_into(&mut data.username);
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/v2/totplogin?<protocol>", data = "<post_data>")]
pub async fn totp_login(
    state: &State<ServerState>,
    db: &GlobedDb,
    ip: IpAddr,
    cfip: CloudflareIPGuard,
    _user_agent: ClientUserAgentGuard<'_>,
    mut post_data: EncryptedJsonGuard<TotpLoginData>,
    protocol: u16,
) -> WebResult<String> {
    check_maintenance!(state);
    check_protocol!(protocol);

    // trim spaces at the end of the name
    trim_name(&mut post_data.0.account_data);

    let state_ = state.state_read().await;
    let account_data = &post_data.0.account_data;

    check_ip(ip, &cfip, state_.config.cloudflare_protection)?;

    if state_.config.userlist_mode == UserlistMode::Whitelist {
        if !db.get_user(account_data.account_id).await?.map_or(false, |x| x.is_whitelisted) {
            unauthorized!("This server has whitelist enabled and your account has not been approved.");
        }
    } else {
        let ban_reason = state_.is_banned(db, account_data.account_id).await;
        if let Err(err) = ban_reason {
            bad_request!(&format!("server error: {err}"));
        }

        if let Some(reason) = ban_reason.unwrap() {
            unauthorized!(&format!("Banned from the server: {reason}"));
        }
    }

    let uak_decoded = b64e::URL_SAFE.decode(post_data.0.authkey)?;
    let valid_authkey = state_.generate_hashed_authkey(account_data.account_id, account_data.user_id, &account_data.username);

    let valid = uak_decoded.len() == valid_authkey.len() && uak_decoded.iter().zip(valid_authkey.iter()).all(|(c1, c2)| *c1 == *c2);

    if !valid {
        unauthorized!("login failed");
    }

    let token = state_
        .token_issuer
        .generate(account_data.account_id, account_data.user_id, &account_data.username);

    debug!("totp login from {} ({}) successful", account_data.account_id, &account_data.username);

    Ok(token)
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/v2/challenge/new?<protocol>", data = "<post_data>")]
pub async fn challenge_new(
    state: &State<ServerState>,
    db: &GlobedDb,
    ip: IpAddr,
    cfip: CloudflareIPGuard,
    _user_agent: ClientUserAgentGuard<'_>,
    mut post_data: EncryptedJsonGuard<AccountData>,
    protocol: u16,
) -> WebResult<String> {
    check_maintenance!(state);
    check_protocol!(protocol);

    // trim spaces at the end of the name
    trim_name(&mut post_data.0);

    let mut state = state.state_write().await;
    let account_data = &post_data.0;

    let user_ip = check_ip(ip, &cfip, state.config.cloudflare_protection)?;

    if state.config.userlist_mode == UserlistMode::Whitelist {
        if !db.get_user(account_data.account_id).await?.map_or(false, |x| x.is_whitelisted) {
            unauthorized!("This server has whitelist enabled and your account has not been approved.");
        }
    } else {
        let ban_reason = state.is_banned(db, account_data.account_id).await;
        if let Err(err) = ban_reason {
            bad_request!(&format!("server error: {err}"));
        }

        if let Some(reason) = ban_reason.unwrap() {
            unauthorized!(&format!("Banned from the server: {reason}"));
        }
    }

    let current_time = SystemTime::now();

    let mut should_return_existing = false;
    // check if there already is a challenge
    if let Some(challenge) = state.active_challenges.get(&ip) {
        // if it's the same account ID then it's OK, return the same challenge
        if challenge.account_id == account_data.account_id && challenge.user_id == account_data.user_id && challenge.name == account_data.username {
            should_return_existing = true;
        } else {
            let passed_time = current_time.duration_since(challenge.started).unwrap_or_default();
            // if it hasn't expired yet, throw an error
            if passed_time.as_secs() < u64::from(state.config.challenge_expiry) {
                trace!("rejecting start, challenge already requested: {user_ip}");
                unauthorized!("challenge already requested for this account ID, please wait a minute and try again");
            }
        }
    }

    if should_return_existing {
        let rand_string = state.active_challenges.get(&user_ip).unwrap().value.clone();
        let verify = state.config.use_gd_api;
        let gd_api_account = state.config.gd_api_account;
        let pubkey = b64e::STANDARD.encode(state.challenge_pubkey);
        let use_secure_mode = std::env::var("GLOBED_GS_SECURE_MODE_KEY").is_ok();

        trace!("sending existing challenge to {user_ip} with {rand_string}");

        return Ok(format!(
            "{}:{}:{}:{}",
            if verify { gd_api_account.to_string() } else { "none".to_string() },
            rand_string,
            pubkey,
            if use_secure_mode { "1" } else { "0" }
        ));
    }

    let challenge: String = match state.create_challenge(
        account_data.account_id,
        account_data.user_id,
        &account_data.username,
        user_ip,
        current_time,
    ) {
        Ok(x) => x,
        Err(err) => {
            warn!("failed to create challenge: {err}");
            bad_request!("failed to create challenge: {err}");
        }
    };

    let pubkey = b64e::STANDARD.encode(state.challenge_pubkey);

    let verify = state.config.use_gd_api;
    let gd_api_account: i32 = state.config.gd_api_account;

    let use_secure_mode = std::env::var("GLOBED_GS_SECURE_MODE_KEY").is_ok();

    Ok(format!(
        "{}:{}:{}:{}",
        if verify { gd_api_account.to_string() } else { "none".to_string() },
        challenge,
        pubkey,
        if use_secure_mode { "1" } else { "0" }
    ))
}

fn decrypt_trust_token(token: &str, key: &str) -> WebResult<Vec<u8>> {
    let encrypted: Vec<u8> = b64e::URL_SAFE.decode(token)?;
    let key = hex::decode(key)?;

    // check sizes
    if encrypted.len() < 24 || key.len() != 32 {
        bad_request!("invalid trust token or key size");
    }

    let mut key_arr = [0u8; 32];
    key_arr.copy_from_slice(&key);

    let cipher = globed_shared::crypto_secretbox::XSalsa20Poly1305::new((&key_arr).into());
    let nonce = encrypted[..24].into();
    let ciphertext = &encrypted[24..];

    let decrypted = cipher.decrypt(nonce, ciphertext)?;

    Ok(decrypted)
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/v2/challenge/verify", data = "<post_data>")]
pub async fn challenge_verify(
    state: &State<ServerState>,
    ip: IpAddr,
    cfip: CloudflareIPGuard,
    _user_agent: ClientUserAgentGuard<'_>,
    mut post_data: EncryptedJsonGuard<ChallengeFinishData>,
) -> WebResult<String> {
    check_maintenance!(state);

    // trim spaces at the end of the name
    trim_name(&mut post_data.0.account_data);

    let state_ = state.state_read().await;
    let account_data = &post_data.0.account_data;

    let user_ip = check_ip(ip, &cfip, state_.config.cloudflare_protection)?;

    trace!("challenge finish: {:?}", post_data.0);

    let challenge: ActiveChallenge = match state_.active_challenges.get(&user_ip) {
        None => {
            unauthorized!("challenge does not exist for this IP address");
        }
        Some(x) => x,
    }
    .clone();

    if challenge.account_id != account_data.account_id {
        warn!(
            "failed to validate challenge: requested for accountid {} but {} completed",
            challenge.account_id, account_data.account_id
        );
        unauthorized!("challenge was requested for a different account id, not validating");
    }

    if challenge.user_id != account_data.user_id {
        warn!(
            "failed to validate challenge ({}): requested for userid {} but {} completed",
            account_data.account_id, challenge.user_id, account_data.user_id
        );
        unauthorized!("challenge was requested for a different user id, not validating");
    }

    if !challenge.name.eq_ignore_ascii_case(&account_data.username) {
        warn!(
            "failed to validate challenge ({}): requested for {} but {} completed",
            account_data.account_id, challenge.name, account_data.username
        );
        unauthorized!("challenge was requested for a different account name, not validating");
    }

    let result = state_.verify_challenge(&challenge, &post_data.0.answer);

    if !result {
        unauthorized!("incorrect challenge solution was provided");
    }

    let use_gd_api = state_.config.use_gd_api;

    // drop the state because `verify_account` can intentionally block for a few seconds
    drop(state_);

    let message_id = if use_gd_api {
        let result = state
            .inner
            .verifier
            .verify_account(account_data.account_id, account_data.user_id, &account_data.username, &post_data.0.answer)
            .await;

        match result {
            Ok(id) => Some(id),
            Err(err) => unauthorized!(&err),
        }
    } else {
        None
    };

    // verify trust token
    let trust_token = if let Ok(sm_key) = std::env::var("GLOBED_GS_SECURE_MODE_KEY") {
        if let Some(trust_token) = &post_data.0.trust_token {
            let decrypted = String::from_utf8(decrypt_trust_token(&trust_token, &sm_key)?)?;
            if let Some((s_value, token_rest)) = decrypted.split_once('|') {
                if s_value != challenge.value {
                    unauthorized!("security check failed: trust token value mismatch");
                }

                Some(token_rest.to_owned())
            } else {
                unauthorized!("security check failed: trust token value mismatch");
            }
        } else {
            unauthorized!("security check failed: trust token missing but required");
        }
    } else {
        None
    };

    if let Some(trust_token) = trust_token {
        info!(
            "generated authkey for {} ({} / {}), trust token: {}",
            account_data.username, account_data.account_id, account_data.user_id, trust_token
        );
    } else {
        info!(
            "generated authkey for {} ({} / {})",
            account_data.username, account_data.account_id, account_data.user_id
        );
    }

    let mut state_ = state.state_write().await;
    state_.active_challenges.remove(&user_ip);
    let authkey = state_.generate_authkey(account_data.account_id, account_data.user_id, &account_data.username);

    Ok(format!(
        "{}:{}",
        message_id.map_or_else(|| "none".to_owned(), |x| x.to_string()),
        b64e::STANDARD.encode(authkey)
    ))
}
