use globed_shared::{GameServerBootData, PROTOCOL_VERSION};
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

pub async fn boot(context: &mut Context<ServerState>) -> roa::Result {
    let password = &*context.must_query("pw")?;
    let correct = context
        .state_read()
        .await
        .config
        .game_server_password
        .clone();

    if !verify_password(password, &correct) {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    let bdata = GameServerBootData {
        protocol: PROTOCOL_VERSION,
    };

    let bdata = serde_json::to_string(&bdata)?;

    context.write(bdata);

    Ok(())
}

pub async fn verify_token(context: &mut Context<ServerState>) -> roa::Result {
    let password = &*context.must_query("pw")?;
    let correct = context
        .state_read()
        .await
        .config
        .game_server_password
        .clone();

    if !verify_password(password, &correct) {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    let account_id = &*context.must_query("account_id")?;
    let token = &*context.must_query("token")?;

    let result = context.state_read().await.verify_token(account_id, token);

    match result {
        Ok(_) => {
            context.write("ok");
        }
        Err(e) => {
            context.write(e.to_string());
        }
    }

    Ok(())
}
