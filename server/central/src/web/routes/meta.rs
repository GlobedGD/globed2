use globed_shared::PROTOCOL_VERSION;
use rand::Rng;
use roa::{preload::PowerBody, throw, Context};

use crate::state::ServerState;

pub async fn version(context: &mut Context<ServerState>) -> roa::Result {
    context.write(PROTOCOL_VERSION.to_string());
    Ok(())
}

pub async fn index(context: &mut Context<ServerState>) -> roa::Result {
    if rand::thread_rng().gen_ratio(1, 100) {
        context.resp.headers.append("Location", "/amoung_pequeno".parse()?);
        throw!(roa::http::StatusCode::TEMPORARY_REDIRECT);
    }
    context.write("hi there cutie :3");
    Ok(())
}

pub async fn servers(context: &mut Context<ServerState>) -> roa::Result {
    let serialized = serde_json::to_string(&context.state_read().await.config.game_servers)?;
    context.write(serialized);
    Ok(())
}

pub async fn pagina_grande(context: &mut Context<ServerState>) -> roa::Result {
    let html = include_str!("page.html");
    context
        .resp
        .headers
        .append(roa::http::header::CACHE_CONTROL, "public, max-age=86400".parse()?);

    context.resp.headers.append("pagina", "grande".parse()?);

    context.write(html);

    context.resp.headers.remove("Content-Type");
    context.resp.headers.append("Content-Type", "text/html".parse()?);
    Ok(())
}
