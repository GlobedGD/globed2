use std::sync::OnceLock;

use globed_shared::PROTOCOL_VERSION;
use rand::Rng;
use roa::{preload::PowerBody, throw, Context};

use crate::state::ServerState;

use super::check_maintenance;

pub async fn version(context: &mut Context<ServerState>) -> roa::Result {
    check_maintenance!(context);
    context.write(PROTOCOL_VERSION.to_string());
    Ok(())
}

pub async fn index(context: &mut Context<ServerState>) -> roa::Result {
    if rand::thread_rng().gen_ratio(1, 0xaa) {
        return _check(context);
    }

    context.write("hi there cutie :3");
    Ok(())
}

pub async fn servers(context: &mut Context<ServerState>) -> roa::Result {
    check_maintenance!(context);
    let serialized = serde_json::to_string(&context.state_read().await.config.game_servers)?;
    context.write(serialized);
    Ok(())
}

fn _check(context: &mut Context<ServerState>) -> roa::Result {
    static VALS: OnceLock<(String, String, String, String)> = OnceLock::new();

    let vals = VALS.get_or_init(|| {
        let cxx = include_bytes!("._x").iter().map(|b| b ^ 0xda).collect::<Vec<_>>();
        (
            String::from_utf8_lossy(&cxx[0..9]).to_string(),
            String::from_utf8_lossy(&cxx[9..15]).to_string(),
            String::from_utf8_lossy(&cxx[15..21]).to_string(),
            String::from_utf8_lossy(&cxx[21..]).to_string(),
        )
    });

    context
        .resp
        .headers
        .append(roa::http::header::CACHE_CONTROL, "public, max-age=1".parse()?);

    context.resp.headers.append(vals.1.as_str(), vals.2.parse()?);

    context.write(vals.3.as_str());

    context.resp.headers.remove("Content-Type");
    context.resp.headers.append("Content-Type", vals.0.parse()?);
    Ok(())
}
