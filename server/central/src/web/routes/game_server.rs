use std::net::IpAddr;

use globed_shared::{
    esp::{types::FastString, ByteBuffer, ByteBufferExtWrite},
    logger::debug,
    GameServerBootData, UserEntry, PROTOCOL_VERSION, SERVER_MAGIC,
};

use rocket::{get, post, State};

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
        protocol: PROTOCOL_VERSION,
        tps: config.tps,
        maintenance: config.maintenance,
        secret_key2: config.secret_key2.clone(),
        token_expiry: config.token_expiry,
        status_print_interval: config.status_print_interval,
        admin_key: FastString::from_str(&config.admin_key),
        whitelist: config.userlist_mode == UserlistMode::Whitelist,
        admin_webhook_url: config.admin_webhook_url.clone(),
    };

    debug!("boot data request from game server {} at {}", user_agent.0, ip_address);

    let mut bb = ByteBuffer::new();
    bb.write_bytes(SERVER_MAGIC);
    bb.write_value(&bdata);

    drop(state);
    Ok(bb.into_vec())
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

    let user = if let Ok(account_id) = user.parse::<i32>() {
        database
            .get_user(account_id)
            .await?
            .unwrap_or_else(|| UserEntry::new(account_id))
    } else {
        let user = database.get_user_by_name(user).await?;

        if let Some(user) = user {
            user
        } else {
            bad_request!("failed to find the user by name");
        }
    };

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/gs/user/update?", data = "<userdata>")]
pub async fn update_user(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<UserEntry>,
) -> WebResult<()> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    database.update_user(userdata.0.account_id, &userdata.0).await?;

    Ok(())
}
