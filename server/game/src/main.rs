#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::cast_possible_truncation,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::wildcard_imports
)]

use std::{collections::HashMap, error::Error};

use anyhow::anyhow;
use globed_shared::{GameServerBootData, PROTOCOL_VERSION};
use log::{error, info, warn, LevelFilter};
use server::GameServerConfiguration;
use state::ServerState;

use server::GameServer;

pub mod bytebufferext;
pub mod data;
pub mod logger;
pub mod managers;
pub mod server;
pub mod server_thread;
pub mod state;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(&*logger::LOGGER_INSTANCE).unwrap();

    if std::env::var("GLOBED_GS_LESS_LOG").unwrap_or("0".to_string()) == "1" {
        log::set_max_level(LevelFilter::Warn);
    } else {
        log::set_max_level(if cfg!(debug_assertions) {
            LevelFilter::Trace
        } else {
            LevelFilter::Info
        });
    }

    let mut host_address = String::new();
    let mut central_url = String::new();
    let mut central_pw = String::new();

    let mut args = std::env::args();
    let exe_name = args.next().unwrap(); // skip executable
    let arg_hostaddr = args.next();
    let arg_central = args.next();
    let arg_password = args.next();

    if arg_hostaddr.is_some() {
        host_address = arg_hostaddr.unwrap();
        if arg_central.is_some() {
            central_url = arg_central.unwrap();
            if arg_password.is_some() {
                central_pw = arg_password.unwrap();
            }
        }
    }

    if host_address.is_empty() {
        host_address = std::env::var("GLOBED_GS_ADDRESS").unwrap_or_default();
    }

    if central_url.is_empty() {
        central_url = std::env::var("GLOBED_GS_CENTRAL_URL").unwrap_or_default();
    }

    if central_pw.is_empty() {
        central_pw = std::env::var("GLOBED_GS_CENTRAL_PASSWORD").unwrap_or_default();
    }

    if central_url.is_empty() || central_pw.is_empty() || host_address.is_empty() {
        error!("Some of the configuration values are not set, aborting launch due to misconfiguration.");
        error!("Correct usage: {exe_name} <address> <central-url> <central-password>");
        error!(
            "or use the environment variables 'GLOBED_GS_ADDRESS', 'GLOBED_GS_CENTRAL_URL' and 'GLOBED_GS_CENTRAL_PASSWORD'"
        );

        panic!("aborting due to misconfiguration");
    }

    if central_url != "none" && !central_url.ends_with('/') {
        central_url += "/";
    }

    let client = reqwest::Client::builder()
        .user_agent(format!("globed-game-server/{}", env!("CARGO_PKG_VERSION")))
        .build()
        .unwrap();

    let config = GameServerConfiguration {
        http_client: client,
        central_url,
        central_pw,
    };

    let state = ServerState::new();

    let (gsbd, standalone) = if config.central_url == "none" {
        warn!("Starting in standalone mode, authentication is disabled");

        (
            GameServerBootData {
                protocol: PROTOCOL_VERSION,
                no_chat: Vec::new(),
                special_users: HashMap::new(),
            },
            true,
        )
    } else {
        info!("Retreiving config from the central server..");

        let response = config
            .http_client
            .post(format!("{}{}", config.central_url, "gs/boot"))
            .query(&[("pw", config.central_pw.clone())])
            .send()
            .await?
            .error_for_status()
            .map_err(|e| anyhow!("central server returned an error: {e}"))?;

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

        (boot_data, false)
    };

    let server = Box::leak(Box::new(GameServer::new(host_address, state, gsbd, config, standalone).await));

    server.run().await?;

    Ok(())
}
