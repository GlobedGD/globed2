use std::net::IpAddr;

use globed_shared::{
    esp::{types::FastString, ByteBuffer, ByteBufferExtWrite},
    logger::debug,
    GameServerBootData, ServerUserEntry, MAX_SUPPORTED_PROTOCOL, SERVER_MAGIC,
};

use rocket::{get, post, serde::json::Json, State};

use crate::{config::UserlistMode, db::GlobedDb, state::ServerState, web::*};

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

async fn _get_user(database: &GlobedDb, user: &str) -> WebResult<ServerUserEntry> {
    Ok(if let Ok(account_id) = user.parse::<i32>() {
        database.get_user(account_id).await?.unwrap_or_else(|| ServerUserEntry::new(account_id))
    } else {
        let user = database.get_user_by_name(user).await?;

        if let Some(user) = user {
            user
        } else {
            bad_request!("failed to find the user by name");
        }
    })
}

#[get("/gs/user/<user>")]
pub async fn get_user(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    user: &str,
    _user_agent: GameServerUserAgentGuard<'_>,
) -> WebResult<CheckedEncodableResponder> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    Ok(CheckedEncodableResponder::new(_get_user(database, user).await?))
}

#[post("/gs/user/update", data = "<userdata>")]
pub async fn update_user(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<ServerUserEntry>,
) -> WebResult<()> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    database.update_user_server(userdata.0.account_id, &userdata.0).await?;

    Ok(())
}

/* /gsp/ apis are the same except they use JSON instead of binary encoding */

#[get("/gsp/user/<user>")]
pub async fn p_get_user(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    user: &str,
    _user_agent: GameServerUserAgentGuard<'_>,
) -> WebResult<Json<ServerUserEntry>> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    Ok(Json(_get_user(database, user).await?))
}

#[post("/gsp/user/update", data = "<userdata>")]
pub async fn p_update_user(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: Json<ServerUserEntry>,
) -> WebResult<()> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    database.update_user_server(userdata.0.account_id, &userdata.0).await?;

    Ok(())
}
