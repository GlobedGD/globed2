use globed_shared::{GameServerBootData, PROTOCOL_VERSION};
use log::info;
use reqwest::StatusCode;
use roa::{preload::PowerBody, query::Query, throw, Context};

use crate::state::ServerState;

fn verify_password(pwd1: &str, pwd2: &str) -> bool {
    if pwd1.len() != pwd2.len() {
        return false;
    }

    let bytes1 = pwd1.as_bytes();
    let bytes2 = pwd2.as_bytes();

    let mut result = 0u8;
    for (b1, b2) in bytes1.iter().zip(bytes2.iter()) {
        result |= b1 ^ b2;
    }

    result == 0
}

macro_rules! check_user_agent {
    ($ctx:expr, $ua:ident) => {
        let useragent = $ctx.req.headers.get(roa::http::header::USER_AGENT);
        if useragent.is_none() {
            throw!(StatusCode::UNAUTHORIZED, "what?");
        }

        let $ua = useragent.unwrap().to_str()?;
        if !cfg!(debug_assertions) && !$ua.starts_with("globed-game-server") {
            throw!(StatusCode::UNAUTHORIZED, "mismatched user agent");
        }
    };
}

pub async fn boot(context: &mut Context<ServerState>) -> roa::Result {
    check_user_agent!(context, user_agent);

    let password = &*context.must_query("pw")?;
    let correct = context.state_read().await.config.game_server_password.clone();

    if !verify_password(password, &correct) {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    let bdata = GameServerBootData {
        protocol: PROTOCOL_VERSION,
        no_chat: context.state_read().await.config.no_chat_list.clone(),
    };

    info!(
        "authenticated game server v{} at {}",
        user_agent.split_once('/').unwrap_or_default().1,
        context.remote_addr.ip()
    );

    let bdata = serde_json::to_string(&bdata)?;

    context.write(bdata);

    Ok(())
}

pub async fn verify_token(context: &mut Context<ServerState>) -> roa::Result {
    check_user_agent!(context, _ua);

    let password = &*context.must_query("pw")?;
    let correct = context.state_read().await.config.game_server_password.clone();

    if !verify_password(password, &correct) {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    let account_id = &*context.must_query("account_id")?;
    let token = &*context.must_query("token")?;

    let result = context.state_read().await.verify_token(account_id, token);

    match result {
        Ok(name) => {
            context.write(format!("status_ok:{name}"));
        }
        Err(e) => {
            context.write(e.to_string());
        }
    }

    Ok(())
}
