use super::routes;
use crate::state::ServerState;

use roa::router::{get, post, Router};

pub fn build_router() -> Router<ServerState> {
    Router::new()
        .gate(roa::query::query_parser)
        /* meta */
        .on("/", get(routes::meta::index))
        .on("/version", get(routes::meta::version))
        /* auth */
        .on("/totplogin", post(routes::auth::totp_login))
        .on("/challenge/new", post(routes::auth::challenge_start))
        .on("/challenge/verify", post(routes::auth::challenge_finish))
}
