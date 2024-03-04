pub mod misc;
pub use misc::*;

use rocket::{catch, Request};

#[catch(404)]
pub fn not_found(_req: &Request) -> &'static str {
    "not found"
}

#[catch(422)]
pub fn query_string(_req: &Request) -> &'static str {
    "you are running an old version of the mod, please update the mod in the geode mod list"
}

pub mod routes {
    pub mod auth;
    pub mod game_server;
    pub mod meta;
    pub mod public;

    pub use super::*;
    use rocket::{routes, Route};

    pub fn build_router() -> Vec<Route> {
        routes![
            meta::version,
            meta::servers,
            meta::index,
            game_server::boot,
            game_server::get_user,
            game_server::update_user,
            game_server::p_get_user,
            game_server::p_update_user,
            auth::totp_login,
            auth::challenge_start,
            auth::challenge_finish,
            public::player_counts,
        ]
    }

    macro_rules! check_maintenance {
        ($ctx:expr) => {
            if $ctx.maintenance() {
                return Err(crate::web::misc::MaintenanceResponder {
                    inner: "The server is currently under maintenance, please try connecting again later",
                }
                .into());
            }
        };
    }

    pub(crate) use check_maintenance;
}
