use std::net::IpAddr;

use rocket::{State, post};
use serde::Deserialize;

use super::{auth_common::*, *};

#[derive(Debug, Deserialize)]
pub struct ArgonLoginData {
    account_data: AccountData,
    token: String,
    #[serde(default)]
    trust_token: Option<String>,
}

#[allow(clippy::too_many_arguments, clippy::similar_names, clippy::no_effect_underscore_binding)]
#[post("/v3/argonlogin", data = "<post_data>")]
pub async fn argon_login(
    state: &State<ServerState>,
    db: &GlobedDb,
    ip: IpAddr,
    cfip: CloudflareIPGuard,
    _user_agent: ClientUserAgentGuard<'_>,
    post_data: EncryptedJsonGuard<ArgonLoginData>,
) -> WebResult<String> {
    auth_common::handle_login(
        state,
        db,
        ip,
        cfip,
        post_data.0.account_data,
        LoginData::Argon(post_data.0.token),
        post_data.0.trust_token,
    )
    .await
}
