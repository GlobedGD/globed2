use std::{
    net::IpAddr,
    time::{SystemTime, UNIX_EPOCH},
};

use globed_shared::{
    anyhow::{self, anyhow},
    base64::{engine::general_purpose as b64e, Engine as _},
    logger::*,
    rand::{self, distributions::Alphanumeric, Rng},
};
use rocket::{post, State};

use crate::{
    config::UserlistMode,
    db::GlobedDb,
    ip_blocker::IpBlocker,
    state::{ActiveChallenge, ServerState},
    web::{routes::check_maintenance, *},
};

// if `use_cf_ip_header` is enabled, this macro gets the actual IP address of the user
// from the CF-Connecting-IP header and puts it in $out.
// it also checks if the request is made by actual cloudflare or if the header is just spoofed.
macro_rules! get_user_ip {
    ($state:expr,$ip:expr,$cfip:expr,$out:ident) => {
        let user_ip: anyhow::Result<IpAddr> = if $state.config.cloudflare_protection && !cfg!(debug_assertions) {
            // verify if the actual peer is cloudflare
            if !IpBlocker::instance().is_allowed(&$ip) {
                warn!("blocking unknown non-cloudflare address: {}", $ip);
                unauthorized!("access is denied from this IP address");
            }

            $cfip.0.ok_or(anyhow!("failed to parse the IP header from Cloudflare"))
        } else {
            Ok($ip)
        };

        let $out = match user_ip {
            Ok(x) => x,
            Err(err) => bad_request!(&err.to_string()),
        };
    };
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/totplogin?<aid>&<uid>&<aname>&<code>")]
pub async fn totp_login(
    state: &State<ServerState>,
    aid: i32,
    uid: i32,
    mut aname: &str,
    code: &str,
    db: &GlobedDb,
    ip: IpAddr,
    _ua: ClientUserAgentGuard<'_>,
    cfip: CloudflareIPGuard,
) -> WebResult<String> {
    check_maintenance!(state);

    aname = aname.trim_end();

    let state_ = state.state_read().await;
    get_user_ip!(state_, ip, cfip, _ip);

    if state_.config.userlist_mode == UserlistMode::Whitelist {
        if !db.get_user(aid).await?.map_or(false, |x| x.is_whitelisted) {
            unauthorized!("This server has whitelist enabled and your account ID has not been approved.");
        }
    } else {
        let ban_reason = state_.is_banned(db, aid).await;
        if let Err(err) = ban_reason {
            bad_request!(&format!("server error: {err}"));
        }

        if let Some(reason) = ban_reason.unwrap() {
            unauthorized!(&format!("Banned from the server: {reason}"));
        }
    }

    let authkey = state_.generate_authkey(aid, uid, aname);
    let valid = state_.verify_totp(&authkey, code);

    if !valid {
        unauthorized!("login failed");
    }

    let token = state_.token_issuer.generate(aid, uid, aname);

    debug!("totp login from {} ({}) successful", aname, aid);

    Ok(token)
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/challenge/new?<aid>&<uid>&<aname>")]
pub async fn challenge_start(
    state: &State<ServerState>,
    aid: i32,
    uid: i32,
    mut aname: &str,
    ip: IpAddr,
    db: &GlobedDb,
    _ua: ClientUserAgentGuard<'_>,
    cfip: CloudflareIPGuard,
) -> WebResult<String> {
    check_maintenance!(state);

    aname = aname.trim_end();

    let mut state = state.state_write().await;
    get_user_ip!(state, ip, cfip, user_ip);

    if state.config.userlist_mode == UserlistMode::Whitelist {
        if !db.get_user(aid).await?.map_or(false, |x| x.is_whitelisted) {
            unauthorized!("This server has whitelist enabled and your account ID has not been approved.");
        }
    } else {
        let ban_reason = state.is_banned(db, aid).await;
        if let Err(err) = ban_reason {
            bad_request!(&format!("server error: {err}"));
        }

        if let Some(reason) = ban_reason.unwrap() {
            unauthorized!(&format!("Banned from the server: {reason}"));
        }
    }

    let current_time = SystemTime::now().duration_since(UNIX_EPOCH)?;

    let mut should_return_existing = false;
    // check if there already is a challenge
    if let Some(challenge) = state.active_challenges.get(&user_ip) {
        // if it's the same account ID then it's OK, return the same challenge
        if challenge.account_id == aid && challenge.user_id == uid && challenge.name == aname {
            should_return_existing = true;
        } else {
            let passed_time = current_time - challenge.started;
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
        drop(state);

        trace!("sending existing challenge to {user_ip} with {rand_string}");

        return Ok(format!(
            "{}:{}",
            if verify {
                gd_api_account.to_string()
            } else {
                "none".to_string()
            },
            rand_string
        ));
    }

    let rand_string: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(32)
        .map(char::from)
        .collect();

    let challenge = ActiveChallenge {
        started: current_time,
        value: rand_string.clone(),
        account_id: aid,
        user_id: uid,
        name: aname.to_owned(),
    };

    state.active_challenges.insert(user_ip, challenge);
    let verify = state.config.use_gd_api;
    let gd_api_account = state.config.gd_api_account;

    Ok(format!(
        "{}:{}",
        if verify {
            gd_api_account.to_string()
        } else {
            "none".to_string()
        },
        rand_string
    ))
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/challenge/verify?<aid>&<uid>&<aname>&<answer>&<systime>")]
pub async fn challenge_finish(
    state: &State<ServerState>,
    aid: i32,
    uid: i32,
    mut aname: &str,
    answer: &str,
    systime: u64,
    ip: IpAddr,
    _ua: ClientUserAgentGuard<'_>,
    cfip: CloudflareIPGuard,
) -> WebResult<String> {
    check_maintenance!(state);

    aname = aname.trim_end();

    let state_ = state.state_read().await;
    get_user_ip!(state_, ip, cfip, user_ip);

    let local_time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("clock went backwards!")
        .as_secs();

    // if they didnt pass any time, it's alright, don't verify the clock
    if systime != 0 {
        let time_difference = if systime > local_time {
            systime - local_time
        } else {
            local_time - systime
        };

        if time_difference > 45 {
            unauthorized!(&format!("your system clock seems to be out of sync, please adjust it in your system settings (time difference: {time_difference} seconds)"));
        }
    }

    trace!("challenge finish from '{aname}' ({aid} / userid {uid}) with answer: {answer}");

    let challenge: ActiveChallenge = match state_.active_challenges.get(&user_ip) {
        None => {
            unauthorized!("challenge does not exist for this IP address");
        }
        Some(x) => x,
    }
    .clone();

    if challenge.account_id != aid {
        warn!(
            "failed to validate challenge: requested for accountid {} but {aid} completed",
            challenge.account_id
        );
        unauthorized!("challenge was requested for a different account id, not validating");
    }

    if challenge.user_id != uid {
        warn!(
            "failed to validate challenge ({aid}): requested for userid {} but {uid} completed",
            challenge.user_id
        );
        unauthorized!("challenge was requested for a different user id, not validating");
    }

    if challenge.name != aname {
        warn!(
            "failed to validate challenge ({aid}): requested for {} but {aname} completed",
            challenge.name
        );
        unauthorized!("challenge was requested for a different account name, not validating");
    }

    let result = state_.verify_challenge(&challenge.value, answer);

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
            .verify_account(aid, uid, aname, answer.parse::<u32>()?)
            .await;

        match result {
            Ok(id) => Some(id),
            Err(err) => unauthorized!(&err),
        }
    } else {
        None
    };

    info!("successfully generated an authkey for {} ({})", aname, aid);

    let mut state_ = state.state_write().await;
    state_.active_challenges.remove(&user_ip);
    let authkey = state_.generate_authkey(aid, uid, aname);

    Ok(format!(
        "{}:{}",
        message_id.map_or_else(|| "none".to_owned(), |x| x.to_string()),
        b64e::STANDARD.encode(authkey)
    ))
}
