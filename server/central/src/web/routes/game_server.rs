use std::net::IpAddr;

use globed_shared::{
    esp::{types::FastString, ByteBuffer, ByteBufferExtWrite},
    logger::debug,
    GameServerBootData, UserEntry, PROTOCOL_VERSION, SERVER_MAGIC,
};

use rocket::{get, post, State};

use crate::{config::UserlistMode, db::GlobedDb, state::ServerState, web::*};

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
) -> WebResult<Vec<u8>> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !verify_password(pw, &correct) {
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
    };

    debug!("boot data request from game server {} at {}", user_agent.0, ip_address);

    let mut bb = ByteBuffer::new();
    bb.write_bytes(SERVER_MAGIC);
    bb.write_value(&bdata);

    drop(state);
    Ok(bb.into_vec())
}

#[get("/gs/user/<user>?<pw>")]
pub async fn get_user(
    state: &State<ServerState>,
    pw: &str,
    database: &GlobedDb,
    user: &str,
    _user_agent: GameServerUserAgentGuard<'_>,
) -> WebResult<CheckedEncodableResponder> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !verify_password(pw, &correct) {
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

#[post("/gs/user/update?<pw>", data = "<userdata>")]
pub async fn update_user(
    state: &State<ServerState>,
    pw: &str,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<UserEntry>,
) -> WebResult<()> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !verify_password(pw, &correct) {
        unauthorized!("invalid gameserver credentials");
    }

    database.update_user(userdata.0.account_id, &userdata.0).await?;

    Ok(())
}
