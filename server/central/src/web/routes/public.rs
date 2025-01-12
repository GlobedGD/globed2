use std::time::{Duration, SystemTime};

use crate::{
    db::{GlobedDb, dbimpl::PlayerCountHistoryEntry},
    state::ServerState,
};

use super::*;
use rocket::{State, get, serde::json::Json};
use serde::Serialize;

#[derive(Serialize)]
pub struct PlayerCounts {
    data: Vec<PlayerCountHistoryEntry>,
}

#[get("/public/players?<period>")]
pub async fn player_counts(
    state: &State<ServerState>,
    db: &GlobedDb,
    period: &str,
    cors: rocket_cors::Guard<'_>,
) -> WebResult<rocket_cors::Responder<Json<PlayerCounts>>> {
    // first see if there's anything yet to be inserted in the db
    let history = state.inner.pinger.get_player_count_history();
    if !history.is_empty() {
        db.insert_player_count_history(&history).await?;
    }

    let period = match period {
        "hour" => Duration::from_hours(1),
        "halfday" => Duration::from_hours(12),
        "day" => Duration::from_days(1),
        "week" => Duration::from_days(7),
        _ => Duration::from_hours(24),
    };

    let data = db.fetch_player_count_history(SystemTime::now() - period).await?;

    let data = if data.len() <= 500 {
        Json(PlayerCounts { data })
    } else {
        // we "compress" it into up to individual 400 data points
        // this is such a mess tbh
        let ratio = data.len() / 200;
        let data = data.chunks(ratio).flat_map(|chunk| chunk.iter().take(1).cloned()).collect();

        Json(PlayerCounts { data })
    };

    Ok(cors.responder(data))
}
