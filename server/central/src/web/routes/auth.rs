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
use roa::{http::StatusCode, preload::PowerBody, query::Query, throw, Context};

use crate::{
    config::UserlistMode,
    ip_blocker::IpBlocker,
    state::{ActiveChallenge, ServerState},
    web::routes::check_maintenance,
};

macro_rules! check_user_agent {
    ($ctx:expr, $ua:ident) => {
        let useragent = $ctx.req.headers.get(roa::http::header::USER_AGENT);
        if useragent.is_none() {
            throw!(StatusCode::UNAUTHORIZED, "what?");
        }

        let $ua = useragent.unwrap().to_str()?;
        if !cfg!(debug_assertions) && !$ua.starts_with("globed-geode-xd") {
            throw!(StatusCode::UNAUTHORIZED, "bad request");
        }
    };
}

// if `use_cf_ip_header` is enabled, this macro gets the actual IP address of the user
// from the CF-Connecting-IP header and puts it in $out.
// it also checks if the request is made by actual cloudflare or if the header is just spoofed.
macro_rules! get_user_ip {
    ($state:expr,$context:expr,$out:ident) => {
        let user_ip: anyhow::Result<IpAddr> = if $state.config.cloudflare_protection && !cfg!(debug_assertions) {
            // verify if the actual peer is cloudflare
            if !IpBlocker::instance().is_allowed(&$context.remote_addr.ip()) {
                warn!("blocking unknown non-cloudflare address: {}", $context.remote_addr.ip());
                throw!(StatusCode::UNAUTHORIZED, "access is denied from this IP address");
            }

            let header = $context.req.headers.get("CF-Connecting-IP");
            let ip = header
                .and_then(|val| val.to_str().ok())
                .and_then(|val| val.parse::<IpAddr>().ok());

            ip.ok_or(anyhow!("failed to parse the IP header from Cloudflare"))
        } else {
            Ok($context.remote_addr.ip())
        };

        let $out = match user_ip {
            Ok(x) => x,
            Err(err) => throw!(StatusCode::BAD_REQUEST, err.to_string()),
        };
    };
}

pub async fn totp_login(context: &mut Context<ServerState>) -> roa::Result {
    check_maintenance!(context);
    check_user_agent!(context, _ua);

    let state = context.state_read().await;
    get_user_ip!(state, context, _ip);

    let account_id = context.must_query("aid")?.parse::<i32>()?;
    let user_id = context.must_query("uid")?.parse::<i32>()?;
    let account_name = &*context.must_query("aname")?;
    let code = &*context.must_query("code")?;

    // if account_name.to_lowercase().contains("sevenworks")
    //     || account_name.to_lowercase() == "7works" && rand::thread_rng().gen_ratio(1, 25)
    // {
    //     throw!(StatusCode::IM_A_TEAPOT);
    // }

    if state.should_block(account_id) {
        throw!(
            StatusCode::FORBIDDEN,
            if state.config.userlist_mode == UserlistMode::Blacklist {
                "<cr>You had only one shot.</c>"
            } else {
                "This server has whitelist enabled and your account ID has not been approved."
            }
        );
    };

    let authkey = state.generate_authkey(account_id, user_id, account_name);
    let valid = state.verify_totp(&authkey, code);

    if !valid {
        drop(state);
        throw!(StatusCode::UNAUTHORIZED, "login failed");
    }

    let token = state.token_issuer.generate(account_id, user_id, account_name);
    drop(state);

    debug!("totp login from {} ({}) successful", account_name, account_id);

    context.write(token);

    Ok(())
}

pub async fn challenge_start(context: &mut Context<ServerState>) -> roa::Result {
    check_maintenance!(context);
    check_user_agent!(context, _ua);

    let account_id = context.must_query("aid")?.parse::<i32>()?;
    let user_id = context.must_query("uid")?.parse::<i32>()?;

    let mut state = context.state_write().await;

    if state.should_block(account_id) {
        trace!("rejecting start, user is banned: {account_id}");
        throw!(
            StatusCode::FORBIDDEN,
            if state.config.userlist_mode == UserlistMode::Blacklist {
                "<cr>You had only one shot.</c>"
            } else {
                "This server has whitelist enabled and your account ID has not been approved."
            }
        );
    };

    get_user_ip!(state, context, user_ip);

    let current_time = SystemTime::now().duration_since(UNIX_EPOCH)?;

    let mut should_return_existing = false;
    // check if there already is a challenge
    if let Some(challenge) = state.active_challenges.get(&user_ip) {
        // if it's the same account ID then it's OK, return the same challenge
        if challenge.account_id == account_id {
            should_return_existing = true;
        } else {
            let passed_time = current_time - challenge.started;
            // if it hasn't expired yet, throw an error
            if passed_time.as_secs() < u64::from(state.config.challenge_expiry) {
                trace!("rejecting start, challenge already requested: {user_ip}");
                throw!(
                    StatusCode::FORBIDDEN,
                    "challenge already requested for this account ID, please wait a minute and try again"
                );
            }
        }
    }

    if should_return_existing {
        let rand_string = state.active_challenges.get(&user_ip).unwrap().value.clone();
        let verify = state.config.use_gd_api;
        let gd_api_account = state.config.gd_api_account;
        drop(state);

        trace!("sending existing challenge to {user_ip} with {rand_string}");

        context.write(format!(
            "{}:{}",
            if verify {
                gd_api_account.to_string()
            } else {
                "none".to_string()
            },
            rand_string
        ));
        return Ok(());
    }

    let rand_string: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(32)
        .map(char::from)
        .collect();

    let challenge = ActiveChallenge {
        started: current_time,
        value: rand_string.clone(),
        account_id,
        user_id,
    };

    state.active_challenges.insert(user_ip, challenge);
    let verify = state.config.use_gd_api;
    let gd_api_account = state.config.gd_api_account;

    drop(state);
    context.write(format!(
        "{}:{}",
        if verify {
            gd_api_account.to_string()
        } else {
            "none".to_string()
        },
        rand_string
    ));

    Ok(())
}

pub async fn challenge_finish(context: &mut Context<ServerState>) -> roa::Result {
    check_maintenance!(context);
    check_user_agent!(context, _ua);

    let account_id = context.must_query("aid")?.parse::<i32>()?;
    let user_id = context.must_query("uid")?.parse::<i32>()?;
    let account_name = &*context.must_query("aname")?;

    let ch_answer = &*context.must_query("answer")?;

    let local_time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("clock went backwards!")
        .as_secs();

    let sys_time = context
        .query("systime")
        .and_then(|time| time.parse::<u64>().ok())
        .unwrap_or(0);

    // if they didnt pass any time, it's alright, don't verify the clock
    if sys_time != 0 {
        let time_difference = if sys_time > local_time {
            sys_time - local_time
        } else {
            local_time - sys_time
        };

        if time_difference > 45 {
            throw!(
                StatusCode::BAD_REQUEST,
                format!("your system clock seems to be out of sync, please adjust it in your system settings (time difference: {time_difference} seconds)")
            );
        }
    }

    trace!(
        "challenge finish from {} ({}) with answer: {}",
        account_name,
        account_id,
        ch_answer
    );

    let state = context.state_read().await;
    get_user_ip!(state, context, user_ip);

    let challenge: ActiveChallenge = match state.active_challenges.get(&user_ip) {
        None => {
            throw!(StatusCode::FORBIDDEN, "challenge does not exist for this IP address");
        }
        Some(x) => x,
    }
    .clone();

    if challenge.account_id != account_id || challenge.user_id != user_id {
        throw!(
            StatusCode::UNAUTHORIZED,
            "challenge was requested for a different account/user ID, not validating"
        );
    }

    let result = state.verify_challenge(&challenge.value, ch_answer);

    if !result {
        throw!(StatusCode::UNAUTHORIZED, "incorrect challenge solution was provided");
    }

    let use_gd_api = state.config.use_gd_api;

    // drop the state because `verify_account` can intentionally block for a few seconds
    drop(state);

    let message_id = if use_gd_api {
        let result = context
            .inner
            .verifier
            .verify_account(account_id, user_id, account_name, ch_answer.parse::<u32>()?)
            .await;

        if let Some(id) = result {
            Some(id)
        } else {
            throw!(StatusCode::UNAUTHORIZED, "challenge solution proof not found");
        }
    } else {
        None
    };

    info!("successfully generated an authkey for {} ({})", account_name, account_id);

    let mut state = context.state_write().await;
    state.active_challenges.remove(&user_ip);
    let authkey = state.generate_authkey(account_id, user_id, account_name);
    drop(state);

    context.write(format!(
        "{}:{}",
        message_id.map_or_else(|| "none".to_owned(), |x| x.to_string()),
        b64e::STANDARD.encode(authkey)
    ));

    Ok(())
}
