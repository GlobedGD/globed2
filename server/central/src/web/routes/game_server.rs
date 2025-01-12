use std::net::IpAddr;

use globed_shared::{
    GameServerBootData, MAX_SUPPORTED_PROTOCOL, SERVER_MAGIC,
    esp::{ByteBuffer, ByteBufferExtWrite, types::FastString},
    logger::debug,
};

use rocket::{State, post};

use crate::{config::UserlistMode, state::ServerState, web::*};

#[post("/gs/boot")]
pub async fn boot(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    ip_address: IpAddr,
    user_agent: GameServerUserAgentGuard<'_>,
) -> WebResult<Vec<u8>> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    let state = state.state_read().await;
    let config = &state.config;

    let bdata = GameServerBootData {
        protocol: MAX_SUPPORTED_PROTOCOL,
        tps: config.tps,
        maintenance: config.maintenance,
        secret_key2: config.secret_key2.clone(),
        token_expiry: config.token_expiry,
        status_print_interval: config.status_print_interval,
        admin_key: FastString::new(&config.admin_key),
        whitelist: config.userlist_mode == UserlistMode::Whitelist,
        admin_webhook_url: config.admin_webhook_url.clone(),
        rate_suggestion_webhook_url: config.rate_suggestion_webhook_url.clone(),
        featured_webhook_url: config.featured_webhook_url.clone(),
        room_webhook_url: config.room_webhook_url.clone(),
        chat_burst_limit: config.chat_burst_limit,
        chat_burst_interval: config.chat_burst_interval,
        roles: config.roles.clone(),
    };

    debug!("boot data request from game server {} at {}", user_agent.0, ip_address);

    let mut bb = ByteBuffer::new();
    bb.write_bytes(SERVER_MAGIC);
    bb.write_value(&bdata);

    drop(state);
    Ok(bb.into_vec())
}
