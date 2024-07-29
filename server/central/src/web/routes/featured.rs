use globed_shared::{warn, webhook};
use rocket::{get, post, serde::json::Json, State};

use crate::{
    db::{dbimpl::FeaturedLevelPage, FeaturedLevel, GlobedDb},
    state::ServerState,
    web::*,
};

#[get("/flevel/current")]
pub async fn current(db: &GlobedDb, _user_agent: ClientUserAgentGuard<'_>) -> WebResult<Json<FeaturedLevel>> {
    let level = db.get_current_featured_level().await?;

    if let Some(level) = level {
        Ok(Json(level))
    } else {
        not_found!("no current featured level".to_owned());
    }
}

// TODO: deprecate
#[get("/flevel/history?<page>")]
pub async fn history(db: &GlobedDb, _user_agent: ClientUserAgentGuard<'_>, page: usize) -> WebResult<Json<Vec<FeaturedLevel>>> {
    let levels = db.get_featured_level_history(page).await?;

    Ok(Json(levels))
}

#[get("/flevel/historyv2?<page>")]
pub async fn historyv2(db: &GlobedDb, _user_agent: ClientUserAgentGuard<'_>, page: usize) -> WebResult<Json<FeaturedLevelPage>> {
    let levels = db.get_featured_level_history_new(page).await?;

    Ok(Json(levels))
}

// replaces the currently active featured level with a new one
#[post("/flevel/replace?<newlevel>&<rate_tier>&<aid>&<adminpwd>&<levelname>&<levelauthor>&<difficulty>")]
pub async fn replace(
    state: &State<ServerState>,
    db: &GlobedDb,
    _user_agent: ClientUserAgentGuard<'_>,
    newlevel: i32,
    rate_tier: i32,
    aid: i32,
    adminpwd: &str,
    levelname: Option<&str>,
    levelauthor: Option<&str>,
    difficulty: Option<i32>,
) -> WebResult<()> {
    let Some(user) = db.get_user(aid).await? else {
        unauthorized!("unauthorized (account)")
    };

    let pwd_valid = user.admin_password.is_some_and(|x| x == adminpwd) || state.state_read().await.config.admin_key == adminpwd;
    if !pwd_valid {
        unauthorized!("unauthorized (password)");
    }

    // check if any of the user's roles allow editing featured levels
    let state = state.state_read().await;
    let has_perm = user.user_roles.iter().any(|role| {
        if let Some(matched_role) = state.config.roles.iter().find(|r| r.id == *role) {
            matched_role.edit_featured_levels || matched_role.admin
        } else {
            false
        }
    });

    if !has_perm {
        unauthorized!("unauthorized (perms)");
    }

    db.replace_featured_level(aid, newlevel, rate_tier).await?;

    // send a webhook message
    let url = state.config.featured_webhook_url.clone();
    if let Err(e) = webhook::send_webhook_messages(
        state.http_client.clone(),
        &url,
        &[webhook::WebhookMessage::LevelFeatured(
            levelname.unwrap_or("<unknown>").to_owned(),
            newlevel,
            levelauthor.unwrap_or("<unknown>").to_owned(),
            difficulty.unwrap_or(0),
            rate_tier,
        )],
        "<add ping here> A new level has been featured in Globed!".into()
    )
    .await
    {
        warn!("error sending webhook message: {e:?}");
    }

    Ok(())
}
