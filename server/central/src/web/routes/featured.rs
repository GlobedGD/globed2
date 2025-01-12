use globed_shared::{warn, webhook};
use rocket::{State, get, post, serde::json::Json};
use serde::Deserialize;

use crate::{
    db::{FeaturedLevel, GlobedDb, dbimpl::FeaturedLevelPage},
    state::ServerState,
    web::*,
};

#[derive(Debug, Deserialize)]
pub struct LevelReplaceData {
    level_id: i32,
    rate_tier: i32,
    account_id: i32,
    admin_password: String,
    level_name: String,
    level_author: String,
    difficulty: i32,
}

#[get("/flevel/current")]
pub async fn current(db: &GlobedDb, _user_agent: ClientUserAgentGuard<'_>) -> WebResult<Json<FeaturedLevel>> {
    let level = db.get_current_featured_level().await?;

    if let Some(level) = level {
        Ok(Json(level))
    } else {
        not_found!("no current featured level".to_owned());
    }
}

#[get("/flevel/historyv2?<page>")]
pub async fn history(db: &GlobedDb, _user_agent: ClientUserAgentGuard<'_>, page: usize) -> WebResult<Json<FeaturedLevelPage>> {
    let levels = db.get_featured_level_history_new(page).await?;

    Ok(Json(levels))
}

async fn do_flevel_replace(state: &State<ServerState>, db: &GlobedDb, data: LevelReplaceData) -> WebResult<()> {
    let Some(user) = db.get_user(data.account_id).await? else {
        unauthorized!("unauthorized (account)")
    };

    let valid_global_pwd = state.state_read().await.config.admin_key == data.admin_password;
    let pwd_valid = user.verify_password(&data.admin_password).unwrap_or(false) || valid_global_pwd;

    if !pwd_valid {
        unauthorized!("unauthorized (password)");
    }

    // check if any of the user's roles allow editing featured levels
    let state = state.state_read().await;
    let has_perm = valid_global_pwd
        || user.user_roles.iter().any(|role| {
            if let Some(matched_role) = state.config.roles.iter().find(|r| r.id == *role) {
                matched_role.edit_featured_levels || matched_role.admin
            } else {
                false
            }
        });

    let webhook_url = state.config.featured_webhook_url.clone();
    let http_client = state.http_client.clone();
    let webhook_message = state.config.featured_webhook_message.clone();
    drop(state);

    if !has_perm {
        unauthorized!("unauthorized (perms)");
    }

    if db.has_been_featured(data.level_id).await? {
        bad_request!("level has already been featured");
    }

    db.replace_featured_level(data.account_id, data.level_id, data.rate_tier).await?;

    // send a webhook message
    if !webhook_url.is_empty() {
        if let Err(e) = webhook::send_webhook_messages(
            http_client.clone(),
            &webhook_url,
            &[webhook::WebhookMessage::LevelFeatured(
                data.level_name,
                data.level_id,
                data.level_author,
                data.difficulty,
                data.rate_tier,
            )],
            Some(webhook_message),
        )
        .await
        {
            warn!("error sending webhook message: {e:?}");
        }
    }

    Ok(())
}

// replaces the currently active featured level with a new one
#[post("/v2/flevel/replace", data = "<data>")]
pub async fn replace(
    state: &State<ServerState>,
    db: &GlobedDb,
    _user_agent: ClientUserAgentGuard<'_>,
    data: EncryptedJsonGuard<LevelReplaceData>,
) -> WebResult<()> {
    do_flevel_replace(state, db, data.0).await
}
