use roa::{preload::PowerBody, Context};

use crate::{state::ServerState, PROTOCOL_VERSION};

pub async fn version(context: &mut Context<ServerState>) -> roa::Result {
    context.write(PROTOCOL_VERSION);
    Ok(())
}

pub async fn index(context: &mut Context<ServerState>) -> roa::Result {
    context.write("hi there cutie :3");
    Ok(())
}
