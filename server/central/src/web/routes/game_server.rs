use globed_shared::{
    esp::{types::FastString, ByteBuffer, ByteBufferExtWrite},
    logger::debug,
    GameServerBootData, PROTOCOL_VERSION, SERVER_MAGIC,
};
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
            throw!(StatusCode::UNAUTHORIZED, "bad request");
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

    let state = context.state_read().await;
    let config = &state.config;

    let bdata = GameServerBootData {
        protocol: PROTOCOL_VERSION,
        no_chat: config.no_chat_list.clone(),
        special_users: config.special_users.clone(),
        tps: config.tps,
        maintenance: config.maintenance,
        secret_key2: config.secret_key2.clone(),
        token_expiry: config.token_expiry,
        admin_key: FastString::from_str(&config.admin_key),
    };

    debug!(
        "boot data request from game server v{} at {}",
        user_agent.split_once('/').unwrap_or_default().1,
        context.remote_addr.ip()
    );

    let mut bb = ByteBuffer::new();
    bb.write_bytes(SERVER_MAGIC);
    bb.write_value(&bdata);

    drop(state);
    context.write(bb.into_vec());
    context
        .resp
        .headers
        .insert(roa::http::header::CONTENT_TYPE, "application/octet-stream".parse().unwrap());

    Ok(())
}
