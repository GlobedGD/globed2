use globed_shared::{
    data::*,
    logger::debug,
    rand::{self, Rng},
    warn, ServerUserEntry, UserLoginData, UserLoginResponse,
};

use rocket::{get, post, serde::json::Json, State};
use serde::{Deserialize, Serialize};

use crate::{db::GlobedDb, state::ServerState, web::*};

async fn _get_or_insert_user_by_id(database: &GlobedDb, account_id: i32) -> WebResult<ServerUserEntry> {
    let data = database.get_user(account_id).await?;

    if let Some(data) = data {
        Ok(data)
    } else {
        database.insert_empty_user(account_id).await?;
        Ok(ServerUserEntry::default())
    }
}

async fn _get_user_by_id(database: &GlobedDb, account_id: i32) -> WebResult<ServerUserEntry> {
    let data = database.get_user(account_id).await?;
    Ok(data.unwrap_or_else(|| ServerUserEntry::new(account_id)))
}

async fn _get_user(database: &GlobedDb, user: &str) -> WebResult<ServerUserEntry> {
    if let Ok(account_id) = user.parse::<i32>() {
        _get_user_by_id(database, account_id).await
    } else {
        let user = database.get_user_by_name(user).await?;

        if let Some(user) = user {
            Ok(user)
        } else {
            bad_request!("failed to find the user by name");
        }
    }
}

async fn _user_entry_from_server(user: ServerUserEntry, database: &GlobedDb) -> WebResult<UserEntry> {
    let mut punishments = database.get_users_punishments(&user).await?;

    Ok(user.to_user_entry(punishments[0].take(), punishments[1].take()))
}

/* User lookup & sync roles routes (for discord bot) */

#[derive(Serialize)]
pub struct UserLookupResponse {
    pub account_id: i32,
    pub name: String,
}

#[get("/gsp/lookup?<username>&<link_code>&<bypass>")]
pub async fn p_user_lookup(
    s_state: &State<ServerState>,
    password: GameServerPasswordGuard,
    username: &str,
    link_code: u32,
    bypass: Option<bool>,
) -> WebResult<Json<UserLookupResponse>> {
    let state = s_state.state_read().await;

    if !password.verify(&state.config.game_server_password) {
        unauthorized!("invalid gameserver credentials");
    }

    debug!("link code: {link_code}, bypass: {bypass:?}");

    let response = if let Some(login) = state.get_login(username, if bypass.unwrap_or(false) { None } else { Some(link_code) }) {
        UserLookupResponse {
            account_id: login.account_id,
            name: login.name.clone(),
        }
    } else {
        not_found!("Failed to find user by given username");
    };

    drop(state);

    s_state.state_write().await.remove_login_code(username);

    Ok(Json(response))
}

#[derive(Deserialize)]
pub struct RoleSyncRequest {
    pub account_id: i32,
    pub keep: Vec<String>,
    pub remove: Vec<String>,
}

#[derive(Deserialize)]
pub struct RoleSyncRequestData {
    pub users: Vec<RoleSyncRequest>,
}

#[post("/gsp/sync_roles", data = "<userdata>")]
pub async fn p_sync_roles(
    state: &State<ServerState>,
    database: &GlobedDb,
    password: GameServerPasswordGuard,
    userdata: Json<RoleSyncRequestData>,
) -> WebResult<()> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    let state = state.state_read().await;

    for r_user in &userdata.users {
        let mut user = _get_or_insert_user_by_id(database, r_user.account_id).await?;

        // update username if it's null
        if user.user_name.is_none() {
            if let Some(name) = state.get_username_from_login(r_user.account_id) {
                database.update_username(r_user.account_id, name).await?;
            }
        }

        // add roles in case user doesn't have them
        for role_id in &r_user.keep {
            // check if the role is valid

            if state.config.roles.iter().any(|x| x.id == *role_id) {
                // add the role to the user
                if !user.user_roles.contains(role_id) {
                    user.user_roles.push(role_id.clone());
                }

                // // if this was a single user query, return back the names of the roles the user now received
                // if userdata.users.len() == 1 {
                //     return_val.push(server_role.)
                // }
            } else {
                warn!("attempting to sync invalid role id: {role_id}");
            }
        }

        // remove roles the user shouldn't have
        user.user_roles.retain(|r| !r_user.remove.contains(r));

        // update the roles
        database.update_user_roles(user.account_id as i32, &user.user_roles).await?;
    }

    Ok(())
}

/* Various user endpoints */

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

    let user = _get_user(database, user).await?;
    let user = _user_entry_from_server(user, database).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/gs/userlogin", data = "<userdata>")]
pub async fn user_login(
    state: &State<ServerState>,
    password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<UserLoginData>,
    _user_agent: GameServerUserAgentGuard<'_>,
) -> WebResult<CheckedEncodableResponder> {
    let correct = state.state_read().await.config.game_server_password.clone();

    if !password.verify(&correct) {
        unauthorized!("invalid gameserver credentials");
    }

    let link_code = rand::thread_rng().gen_range(1000..10_000);

    // store login attempt
    state.state_write().await.put_login(&userdata.0.name, userdata.0.account_id, link_code);

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    let ban = if let Some(ban_id) = &user.active_ban {
        match database.get_punishment(*ban_id).await {
            Ok(x) => Some(x),
            Err(sqlx::Error::RowNotFound) => None,
            Err(e) => return Err(e.into()),
        }
    } else {
        None
    };

    let resp = UserLoginResponse {
        user_entry: user,
        ban,
        link_code,
    };

    Ok(CheckedEncodableResponder::new(resp))
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

// TODO gsp user update replacement

// Update endpoints

// TODO webhooks

#[post("/user/update/username", data = "<userdata>")]
pub async fn update_username(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminUpdateUsernameAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database.update_username(userdata.0.account_id, &userdata.0.username).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/name_color", data = "<userdata>")]
pub async fn update_name_color(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminSetNameColorAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database.update_user_name_color(userdata.0.account_id, &userdata.0.color).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/name_color", data = "<userdata>")]
pub async fn update_roles(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminSetUserRolesAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database.update_user_roles(userdata.0.account_id, &userdata.0.roles).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/punish", data = "<userdata>")]
pub async fn update_punish(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminPunishUserAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database.punish_user(&userdata.0).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/unpunish", data = "<userdata>")]
pub async fn update_unpunish(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminRemovePunishmentAction>,
) -> WebResult<CheckedEncodableResponder> {
    database.unpunish_user(userdata.0.account_id, userdata.0.is_ban).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/whitelist", data = "<userdata>")]
pub async fn update_whitelist(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminWhitelistAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database.whitelist_user(userdata.0.account_id, userdata.0.state).await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/adminpw", data = "<userdata>")]
pub async fn update_admin_password(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminSetAdminPasswordAction>,
) -> WebResult<CheckedEncodableResponder> {
    // insert empty user in case it does not exist
    database.insert_empty_user(userdata.0.account_id).await?;

    database
        .update_user_admin_password(userdata.0.account_id, &userdata.0.new_password)
        .await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}

#[post("/user/update/editpunish", data = "<userdata>")]
pub async fn update_edit_punishment(
    _password: GameServerPasswordGuard,
    database: &GlobedDb,
    userdata: CheckedDecodableGuard<AdminEditPunishmentAction>,
) -> WebResult<CheckedEncodableResponder> {
    database
        .edit_punishment(
            userdata.0.account_id,
            userdata.0.is_ban,
            userdata.0.reason.try_to_str(),
            userdata.0.expires_at,
        )
        .await?;

    let user = _get_user_by_id(database, userdata.0.account_id).await?;

    Ok(CheckedEncodableResponder::new(user))
}
