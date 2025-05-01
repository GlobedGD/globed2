use std::net::IpAddr;

use rocket::State;
use serde::Deserialize;

use globed_shared::{
    anyhow::{self, anyhow},
    base64::{Engine as _, engine::general_purpose as b64e},
    crypto_box::aead::Aead,
    crypto_secretbox::{KeyInit, XSalsa20Poly1305},
    logger::*,
};

use super::*;
use crate::{argon_client::Verdict, config::UserlistMode, ip_blocker::IpBlocker};

#[derive(Debug, Deserialize)]
pub struct AccountData {
    pub account_id: i32,
    pub user_id: i32,
    pub username: String,
}

pub fn check_ip(ip: IpAddr, cfip: &CloudflareIPGuard, cloudflare: bool) -> WebResult<IpAddr> {
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

pub fn trim_name(data: &mut AccountData) {
    let trimmed = data.username.trim_end();
    trimmed.to_owned().clone_into(&mut data.username);
}

pub fn decrypt_trust_token(token: &str, key: &str) -> WebResult<Vec<u8>> {
    let encrypted: Vec<u8> = b64e::URL_SAFE.decode(token)?;
    let key = hex::decode(key)?;

    // check sizes
    if encrypted.len() < 24 || key.len() != 32 {
        bad_request!("invalid trust token or key size");
    }

    let mut key_arr = [0u8; 32];
    key_arr.copy_from_slice(&key);

    let cipher = XSalsa20Poly1305::new((&key_arr).into());
    let nonce = encrypted[..24].into();
    let ciphertext = &encrypted[24..];

    let decrypted = cipher.decrypt(nonce, ciphertext)?;

    Ok(decrypted)
}

pub enum LoginData {
    Old(String),   // authkey
    Argon(String), // argon token
}

#[inline]
pub async fn handle_login(
    state: &State<ServerState>,
    db: &GlobedDb,
    ip: IpAddr,
    cfip: CloudflareIPGuard,
    mut account_data: AccountData,
    login_data: LoginData,
    trust_token: Option<String>,
) -> WebResult<String> {
    check_maintenance!(state);

    // trim spaces at the end of the name
    trim_name(&mut account_data);

    let state_ = state.state_read().await;

    let user_ip = check_ip(ip, &cfip, state_.config.cloudflare_protection)?;

    if state_.config.userlist_mode == UserlistMode::Whitelist && !db.get_user(account_data.account_id).await?.is_some_and(|x| x.is_whitelisted) {
        unauthorized!("This server has whitelist enabled and your account has not been approved.");
    }

    // validate
    let trust_token = match login_data {
        LoginData::Old(ref authkey) => {
            let uak_decoded = b64e::URL_SAFE.decode(authkey)?;
            let valid_authkey = state_.generate_hashed_authkey(account_data.account_id, account_data.user_id, &account_data.username);

            let valid = uak_decoded.len() == valid_authkey.len() && uak_decoded.iter().zip(valid_authkey.iter()).all(|(c1, c2)| *c1 == *c2);

            if !valid {
                unauthorized!("login failed");
            }

            None
        }

        LoginData::Argon(ref token) => {
            let client = match state_.argon_client.as_ref() {
                Some(x) => x,
                None => bad_request!("this server does not have argon authentication enabled"),
            };

            match client
                .validate_token(account_data.account_id, account_data.user_id, &account_data.username, token)
                .await
            {
                Ok(Verdict::Strong) => {}

                // if skip name check is enabled, consider a weak token as valid
                Ok(Verdict::Weak(_username)) if state_.config.skip_name_check => {}

                Ok(Verdict::Weak(username)) => {
                    debug!(
                        "[{user_ip}] invalid token due to username mismatch: '{}' vs '{}'",
                        account_data.username, username
                    );
                    unauthorized!("invalid username, please try to open GD account settings and Refresh Login");
                }

                Ok(Verdict::Invalid(cause)) => {
                    warn!("[{user_ip}] invalid token: {cause}");
                    unauthorized!(&format!("token validation failure: {cause}"));
                }

                Err(err) => {
                    warn!("[{user_ip}] failed to validate argon token: {err}");
                    bad_request!("failed to validate token, argon server returned an error or could not be reached");
                }
            }

            // validate trust token
            if let Ok(sm_key) = std::env::var("GLOBED_GS_SECURE_MODE_KEY")
                && !sm_key.is_empty()
            {
                if let Some(trust_token) = &trust_token {
                    let decrypted = String::from_utf8(decrypt_trust_token(trust_token, &sm_key)?)?;

                    if let Some((account_id, token_rest)) = decrypted.split_once('|') {
                        if account_id.parse::<i32>().unwrap_or(0) != account_data.account_id {
                            unauthorized!("security check failed: trust token value mismatch");
                        }

                        Some(token_rest.to_owned())
                    } else {
                        unauthorized!("security check failed: trust token value mismatch");
                    }
                } else {
                    // unauthorized!("security check failed: trust token missing but required");
                    None
                }
            } else {
                None
            }
        }
    };

    let argon_str = match login_data {
        LoginData::Old(_) => "old",
        LoginData::Argon(_) => "argon",
    };

    if let Some(trust_token) = trust_token {
        debug!(
            "[{} ({}) @ {}] login successful ({argon_str}), trust token: {trust_token}",
            account_data.username, account_data.account_id, user_ip
        );
    } else {
        debug!(
            "[{} ({}) @ {}] login successful ({argon_str})",
            account_data.username, account_data.account_id, user_ip
        );
    }

    let token = state_
        .token_issuer
        .generate(account_data.account_id, account_data.user_id, &account_data.username);

    Ok(token)
}
