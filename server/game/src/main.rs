use std::{error::Error, sync::Arc};

use globed_shared::{GameServerBootData, PROTOCOL_VERSION};
use log::{error, info, LevelFilter};
use logger::Logger;
use state::ServerStateData;
use tokio::sync::RwLock;

use server::GameServer;

pub mod bytebufferext;
pub mod data;
pub mod logger;
pub mod server;
pub mod server_thread;
pub mod state;

static LOGGER: Logger = Logger;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(&LOGGER).unwrap();

    if std::env::var("GLOBED_GS_LESS_LOG").unwrap_or("0".to_string()) == "1" {
        log::set_max_level(LevelFilter::Warn);
    } else {
        log::set_max_level(if cfg!(debug_assertions) {
            LevelFilter::Trace
        } else {
            LevelFilter::Info
        });
    }

    let mut host_address: String = "".to_string();
    let mut central_url: String = "".to_string();
    let mut central_pw: String = "".to_string();

    let mut args = std::env::args();
    let exe_name = args.next().unwrap(); // skip executable
    let arg1 = args.next();
    let arg2 = args.next();
    let arg3 = args.next();

    if arg1.is_some() {
        host_address = arg1.unwrap();
        if arg2.is_some() {
            central_url = arg2.unwrap();
            if arg3.is_some() {
                central_pw = arg3.unwrap();
            }
        }
    }

    if host_address.is_empty() {
        host_address = std::env::var("GLOBED_GS_ADDRESS").unwrap_or("".to_string());
    }

    if central_url.is_empty() {
        central_url = std::env::var("GLOBED_GS_CENTRAL_URL").unwrap_or("".to_string());
    }

    if central_pw.is_empty() {
        central_pw = std::env::var("GLOBED_GS_CENTRAL_PASSWORD").unwrap_or("".to_string());
    }

    if central_url.is_empty() || central_pw.is_empty() {
        error!("Some of the configuration values are not set, aborting launch due to misconfiguration.");
        error!("Correct usage: {exe_name} <address> <central-url> <central-password>");
        error!(
            "or use the environment variables 'GLOBED_GS_ADDRESS', 'GLOBED_GS_CENTRAL_URL' and 'GLOBED_GS_CENTRAL_PASSWORD'"
        );

        panic!("aborting due to misconfiguration");
    }

    if !central_url.ends_with('/') {
        central_url += "/";
    }

    let client = reqwest::Client::builder()
        .user_agent(format!("globed-game-server/{}", env!("CARGO_PKG_VERSION")))
        .build()
        .unwrap();

    let state = Arc::new(RwLock::new(ServerStateData {
        http_client: client,
        central_url: central_url.clone(),
        central_pw: central_pw.clone(),
    }));

    info!("Retreiving config from the central server..");

    let state_g = state.read().await;
    let response = state_g
        .http_client
        .post(format!("{}{}", state_g.central_url, "gs/boot"))
        .query(&[("pw", state_g.central_pw.clone())])
        .send()
        .await?
        .error_for_status()?;

    drop(state_g);

    let configuration = response.text().await?;
    let boot_data: GameServerBootData = serde_json::from_str(&configuration)?;

    if boot_data.protocol != PROTOCOL_VERSION {
        error!("Incompatible protocol versions!");
        error!(
            "This game server is on {}, while the central server uses {}",
            PROTOCOL_VERSION, boot_data.protocol
        );
        panic!("aborting due to incompatible protocol versions");
    }

    let server = Box::leak(Box::new(
        GameServer::new(host_address, state.clone(), boot_data).await,
    ));

    server.run().await?;

    Ok(())
}
