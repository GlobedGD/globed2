use reqwest::StatusCode;
use roa::{preload::PowerBody, query::Query, throw, Context};

use crate::{state::ServerState, PROTOCOL_VERSION};

pub async fn boot(context: &mut Context<ServerState>) -> roa::Result {
    let password = &*context.must_query("pw")?;
    let correct = context
        .state_read()
        .await
        .config
        .game_server_password
        .clone();

    if password.len() != correct.len() {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    let bytes1 = password.as_bytes();
    let bytes2 = correct.as_bytes();

    let mut result = 0u8;
    for (b1, b2) in bytes1.iter().zip(bytes2.iter()) {
        result |= b1 ^ b2;
    }

    if result != 0 {
        throw!(StatusCode::UNAUTHORIZED, "invalid gameserver credentials");
    }

    context.write(PROTOCOL_VERSION);

    Ok(())
}
