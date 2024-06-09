use std::sync::OnceLock;

use globed_shared::{
    base64::{engine::general_purpose as b64e, Engine as _},
    esp::{ByteBuffer, ByteBufferExt, ByteBufferExtWrite},
    rand::{self, Rng},
    MIN_CLIENT_VERSION, PROTOCOL_VERSION, SERVER_MAGIC,
};

use rocket::{
    get,
    http::{ContentType, Status},
    State,
};

use crate::state::ServerState;

use super::*;

#[get("/version")]
pub fn version() -> String {
    PROTOCOL_VERSION.to_string()
}

#[get("/")]
pub fn index() -> (Status, (ContentType, String)) {
    if rand::thread_rng().gen_ratio(1, 0xaa) {
        return _check();
    }

    (Status::Ok, (ContentType::Text, "hi there cutie :3".to_owned()))
}

#[get("/robots.txt")]
pub fn robots() -> String {
    "User-agent: *\nDisallow: /".to_owned()
}

#[get("/servers?<protocol>")]
pub async fn servers(state: &State<ServerState>, protocol: u16) -> WebResult<String> {
    check_maintenance!(state);
    check_protocol!(protocol);

    let state = state.state_read().await;
    let servers = &state.config.game_servers;

    let mut buf = ByteBuffer::with_capacity(servers.len() * 128);
    buf.write_bytes(SERVER_MAGIC);
    buf.write_value(servers);

    drop(state);

    let encoded = b64e::STANDARD.encode(buf.as_bytes());

    Ok(encoded)
}

fn _check() -> (Status, (ContentType, String)) {
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

    (Status::Ok, (ContentType::HTML, vals.3.clone()))
}
