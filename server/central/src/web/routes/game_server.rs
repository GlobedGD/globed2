use std::net::IpAddr;

use globed_shared::{
    esp::{types::FastString, ByteBuffer, ByteBufferExtWrite},
    logger::debug,
    GameServerBootData, PROTOCOL_VERSION, SERVER_MAGIC,
};

use rocket::{post, State};

use crate::state::ServerState;
use crate::web::*;

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

#[post("/gs/boot?<pw>")]
pub async fn boot(
    state: &State<ServerState>,
    pw: &str,
    ip_address: IpAddr,
    user_agent: GameServerUserAgentGuard<'_>,
) -> Result<Vec<u8>, GenericErrorResponder<String>> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !verify_password(pw, &correct) {
        return Err(UnauthorizedResponder::new("invalid gameserver credentials".to_owned()).into());
    }

    let state = state.state_read().await;
    let config = &state.config;

    let bdata = GameServerBootData {
        protocol: PROTOCOL_VERSION,
        no_chat: config.no_chat_list.clone(),
        special_users: config.special_users.clone(),
        tps: config.tps,
        maintenance: config.maintenance,
        secret_key2: config.secret_key2.clone(),
        token_expiry: config.token_expiry,
        status_print_interval: config.status_print_interval,
        admin_key: FastString::from_str(&config.admin_key),
    };

    debug!("boot data request from game server {} at {}", user_agent.0, ip_address);

    let mut bb = ByteBuffer::new();
    bb.write_bytes(SERVER_MAGIC);
    bb.write_value(&bdata);

    drop(state);
    Ok(bb.into_vec())
}
