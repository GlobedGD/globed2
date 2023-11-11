use globed_shared::PROTOCOL_VERSION;
use roa::{preload::PowerBody, Context};

use crate::state::ServerState;

pub async fn version(context: &mut Context<ServerState>) -> roa::Result {
    context.write(PROTOCOL_VERSION.to_string());
    Ok(())
}

pub async fn index(context: &mut Context<ServerState>) -> roa::Result {
    context.write("hi there cutie :3");
    Ok(())
}
