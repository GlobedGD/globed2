pub mod guards;
pub mod responders;

pub use guards::*;
pub use responders::*;

use rocket::{Request, catch};

#[catch(404)]
pub fn not_found(_req: &Request) -> &'static str {
    "not found"
}

#[catch(422)]
pub fn query_string(_req: &Request) -> &'static str {
    use const_format::concatcp;
    use globed_shared::MIN_CLIENT_VERSION;

    concatcp!(
        "Outdated Globed version, minimum required is ",
        MIN_CLIENT_VERSION,
        ". Please make sure that you are on the latest versions of Geometry Dash, Geode and Globed."
    )
}

pub mod routes {
    pub mod auth;
    pub mod featured;
    pub mod game_server;
    pub mod meta;
    pub mod public;
    pub mod user;

    pub use super::*;
    pub use crate::{db::GlobedDb, state::ServerState};
    use rocket::{Route, routes};
    pub use rocket::{State, get, post};

    pub fn build_router() -> Vec<Route> {
        routes![
            meta::version,
            meta::versioncheck,
            meta::servers,
            meta::index,
            meta::robots,
            game_server::boot,
            auth::totp_login,
            auth::challenge_new,
            auth::challenge_verify,
            featured::current,
            featured::history,
            featured::replace,
            public::player_counts,
            user::get_user,
            user::p_get_user,
            user::user_login,
            user::update_username,
            user::update_name_color,
            user::update_roles,
            user::update_punish,
            user::update_unpunish,
            user::update_whitelist,
            user::update_admin_password,
            user::update_edit_punishment,
            user::get_punishment_history,
            user::get_many_user_names,
            user::p_user_lookup,
            user::p_sync_roles,
        ]
    }

    macro_rules! check_maintenance {
        ($ctx:expr) => {
            if $ctx.maintenance() {
                return Err(crate::web::MaintenanceResponder {
                    inner: "The server is currently under maintenance, please try connecting again later",
                }
                .into());
            }
        };
    }

    pub(crate) use check_maintenance;
}
