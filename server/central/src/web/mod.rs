pub mod misc;
pub use misc::*;

pub mod routes {
    pub mod auth;
    pub mod game_server;
    pub mod meta;

    pub use super::*;
    use rocket::{routes, Route};

    pub fn build_router() -> Vec<Route> {
        routes![
            meta::version,
            meta::servers,
            meta::index,
            game_server::boot,
            auth::totp_login,
            auth::challenge_start,
            auth::challenge_finish
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
