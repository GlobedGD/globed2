use std::sync::OnceLock;

use globed_shared::{
    MAX_SUPPORTED_PROTOCOL, MIN_CLIENT_VERSION, MIN_GD_VERSION, MIN_SUPPORTED_PROTOCOL, SERVER_MAGIC,
    base64::{Engine as _, engine::general_purpose as b64e},
    esp::{ByteBuffer, ByteBufferExt, ByteBufferExtWrite},
    rand::{self, Rng},
};

use rocket::{
    State, get,
    http::{ContentType, Status},
    serde::json::Json,
};
use serde::Serialize;

use crate::{
    config::{GameServerEntry, ServerRelay},
    state::ServerState,
};

use super::*;

// Deprecated, but probably will never be removed for legacy reasons lol
#[get("/version")]
pub fn version() -> String {
    // send minimum supported protocol
    MIN_SUPPORTED_PROTOCOL.to_string()
}

#[derive(Serialize)]
pub struct VersionCheckResponse {
    pub pmin: u16,
    pub pmax: u16,
    pub gdmin: &'static str,
    pub globedmin: &'static str,
}

// TODO: remove
#[get("/versioncheck?<gd>&<globed>&<protocol>")]
pub fn versioncheck(gd: &str, globed: &str, protocol: u16) -> WebResult<Json<VersionCheckResponse>> {
    let resp = || VersionCheckResponse {
        pmin: MIN_SUPPORTED_PROTOCOL,
        pmax: MAX_SUPPORTED_PROTOCOL,
        gdmin: MIN_GD_VERSION,
        globedmin: MIN_CLIENT_VERSION,
    };

    if protocol == u16::MAX {
        return Ok(Json(resp()));
    }

    // do validation

    // i hate it but yeah we parse the versions as floats
    if gd != MIN_GD_VERSION {
        let gd_flt = gd.parse::<f32>()?;
        let min_gd_flt = MIN_GD_VERSION.parse::<f32>()?;

        if gd_flt < min_gd_flt && (min_gd_flt - gd_flt > 0.00001) {
            bad_request!(&format!(
                "Outdated Geometry Dash version. This server requires at least v{MIN_GD_VERSION}, and your device is running v{gd}."
            ));
        }
    }

    if protocol < MIN_SUPPORTED_PROTOCOL {
        bad_request!(&format!(
            "Outdated Globed version, minimum required is {MIN_CLIENT_VERSION}, your is {globed}. Please update Globed."
        ));
    } else if protocol > MAX_SUPPORTED_PROTOCOL {
        bad_request!(&format!(
            "Outdated server, client requested protocol version v{protocol}, server supports up to v{MAX_SUPPORTED_PROTOCOL}"
        ));
    }

    Ok(Json(resp()))
}

#[get("/")]
pub fn index() -> (Status, (ContentType, String)) {
    if rand::rng().random_ratio(1, 0xaa) {
        return _check();
    }

    (Status::Ok, (ContentType::Text, "hi there cutie :3".to_owned()))
}

#[get("/robots.txt")]
pub fn robots() -> String {
    "User-agent: *\nDisallow: /".to_owned()
}

// TODO: remove
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

#[derive(Serialize)]
pub struct ServerMeta {
    pub servers: Vec<GameServerEntry>,
    pub supported: bool,
    pub auth: bool,
    pub argon: Option<String>,
    pub gdmin: &'static str,
    pub globedmin: &'static str,
    pub relays: Vec<ServerRelay>,
}

#[get("/v3/meta?<protocol>")]
pub async fn meta(state: &State<ServerState>, protocol: u16) -> WebResult<Json<ServerMeta>> {
    check_maintenance!(state);

    let state = state.state_read().await;
    let servers = &state.config.game_servers;
    let supported = globed_shared::SUPPORTED_PROTOCOLS.contains(&protocol);

    Ok(Json(ServerMeta {
        servers: servers.clone(),
        supported,
        auth: state.config.use_gd_api || state.config.use_argon,
        argon: state.config.use_argon.then(|| state.config.argon_url.clone()),
        gdmin: MIN_GD_VERSION,
        globedmin: MIN_CLIENT_VERSION,
        relays: state.config.relays.clone(),
    }))
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
