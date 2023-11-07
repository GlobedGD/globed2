use std::{
    collections::HashMap,
    time::{SystemTime, UNIX_EPOCH},
};

use base64::{engine::general_purpose as b64e, Engine as _};
use log::warn;
use rand::{distributions::Alphanumeric, Rng};
use reqwest::StatusCode;
use roa::{preload::PowerBody, query::Query, throw, Context};

use crate::state::{ActiveChallenge, ServerState};

pub async fn totp_login(context: &mut Context<ServerState>) -> roa::Result {
    let account_id = &*context.must_query("accountid")?;
    let code = &*context.must_query("code")?;

    let state = context.state_read().await;

    let authkey = state.generate_authkey(&account_id);
    let valid = state.verify_totp(&authkey, code);

    drop(state);

    if !valid {
        throw!(StatusCode::UNAUTHORIZED, "login failed");
    }

    Ok(())
}

pub async fn challenge_start(context: &mut Context<ServerState>) -> roa::Result {
    let account_id = &*context.must_query("accountid")?;
    let account_id = account_id.parse::<i32>()?;

    let mut state = context.state_write().await;

    let current_time = SystemTime::now().duration_since(UNIX_EPOCH)?;

    let mut should_remove = false;
    let mut should_return_existing = false;
    // check if there already is a challenge
    match state.active_challenges.get(&account_id) {
        Some(challenge) => {
            // if it's the same addr then it's OK, return the same challenge
            if challenge.initiator == context.remote_addr.ip() {
                should_return_existing = true;
            } else {
                let passed_time = current_time - challenge.started;
                if passed_time.as_secs() > (state.config.challenge_expiry as u64) {
                    should_remove = true;
                } else {
                    throw!(
                        StatusCode::FORBIDDEN,
                        "challenge already requested for this account ID, please wait a minute and try again"
                    );
                }
            }
        }
        None => {}
    };

    let level_id = state.config.challenge_level;

    if should_return_existing {
        let rand_string = state
            .active_challenges
            .get(&account_id)
            .unwrap()
            .value
            .clone();
        drop(state);

        context.write(format!("{}:{}", level_id, rand_string));
        return Ok(());
    }

    if should_remove {
        state.active_challenges.remove(&account_id);
    }

    let rand_string: String = rand::thread_rng()
        .sample_iter(&Alphanumeric)
        .take(32)
        .map(char::from)
        .collect();

    let challenge = ActiveChallenge {
        started: current_time,
        value: rand_string.clone(),
        initiator: context.remote_addr.ip(),
    };

    state.active_challenges.insert(account_id, challenge);

    drop(state);

    context.write(format!("{}:{}", level_id, rand_string));

    Ok(())
}

pub async fn challenge_finish(context: &mut Context<ServerState>) -> roa::Result {
    let account_id = &*context.must_query("accountid")?;
    let account_id = account_id.parse::<i32>()?;

    let ch_answer = &*context.must_query("answer")?;

    let state = context.state_read().await;

    let challenge = match state.active_challenges.get(&account_id) {
        None => {
            throw!(
                StatusCode::FORBIDDEN,
                "challenge does not exist for this account ID"
            );
        }
        Some(x) => x,
    }
    .clone();

    if challenge.initiator != context.remote_addr.ip() {
        throw!(
            StatusCode::UNAUTHORIZED,
            "challenge was requested by a different IP address, not validating"
        );
    }

    let result = state.verify_challenge(&challenge.value, &ch_answer);

    if !result {
        throw!(
            StatusCode::UNAUTHORIZED,
            "invalid answer to the challenge in the query parameter"
        );
    }

    let challenge_level = state.config.challenge_level;
    drop(state);

    // now we have to request rob's servers and check if the challenge was solved
    // todo rate limiting

    let result = reqwest::ClientBuilder::new()
        .user_agent("")
        .build()?
        .post("http://www.boomlings.com/database/getGJComments21.php")
        .form(&[
            ("levelID", challenge_level.to_string()),
            ("page", "0".to_string()),
            ("secret", "Wmfd2893gb7".to_string()),
            ("gameVersion", "21".to_string()),
            ("binaryVersion", "35".to_string()),
            ("gdw", "0".to_string()),
            ("mode", "0".to_string()),
            ("total", "0".to_string()),
        ])
        .send()
        .await;

    let mut response = match result {
        Err(err) => {
            warn!("Failed to make a request to boomlings: {}", err.to_string());
            throw!(StatusCode::INTERNAL_SERVER_ERROR, err.to_string());
        }
        Ok(x) => x,
    }
    .text()
    .await?;

    let octothorpe = response.find("#");

    if let Some(octothorpe) = octothorpe {
        response = response.split_at(octothorpe).0.to_string();
    }

    let comment_strings = response.split("|");
    for string in comment_strings {
        let colon = string.find(":");
        if colon.is_none() {
            continue;
        }

        let (comment_str, author_str) = string.split_at(colon.unwrap());
        // parse them
        let comment = parse_robtop_string(comment_str);
        let author = parse_robtop_string(&author_str[1..]); // we ignore the colon at the start

        let comment_text = comment.get("2");
        let author_name = author.get("1");
        let author_id = author.get("16");

        if comment_text.is_none() || author_name.is_none() || author_id.is_none() {
            continue;
        }

        let author_id = author_id.unwrap();
        // let author_name = author_name.unwrap();
        let comment_text = comment_text.unwrap();

        if author_id.parse::<i32>()? != account_id {
            continue;
        }

        let decoded = b64e::URL_SAFE.decode(comment_text)?;
        let comment_text = String::from_utf8_lossy(&decoded);

        let mut state = context.state_write().await;

        let result = state.verify_challenge(&challenge.value, &comment_text);

        // on success, delete the challenge and generate the authkey
        if result {
            state.active_challenges.remove(&account_id);
            let authkey = state.generate_authkey(&account_id.to_string());
            drop(state);

            context.write(b64e::STANDARD.encode(authkey));
            return Ok(());
        } else {
            break;
        }
    }

    throw!(
        StatusCode::UNAUTHORIZED,
        "failed to find the comment with the correct challenge solution"
    )
}

fn parse_robtop_string(data: &str) -> HashMap<&str, &str> {
    let separator = '~';

    let pairs: Vec<&str> = data.split(separator).collect();
    let mut map = HashMap::new();

    for i in (0..pairs.len()).step_by(2) {
        if let Some(key) = pairs.get(i) {
            if let Some(value) = pairs.get(i + 1) {
                map.insert(*key, *value);
            }
        }
    }

    map
}
