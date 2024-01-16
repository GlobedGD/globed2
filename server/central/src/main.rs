#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::wildcard_imports
)]

use std::{
    error::Error,
    net::{IpAddr, SocketAddr},
    path::PathBuf,
    time::Duration,
};

use async_watcher::{notify::RecursiveMode, AsyncDebouncer};
use config::ServerConfig;
use globed_shared::logger::{error, info, log, warn, LogLevelFilter, Logger};
use roa::{tcp::Listener, App};
use state::{ServerState, ServerStateData};

pub mod config;
pub mod ip_blocker;
pub mod state;
pub mod web;

fn abort_misconfig() -> ! {
    error!("aborting launch due to misconfiguration.");
    std::process::exit(1);
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(Logger::instance("globed_central_server", false)).unwrap();

    if std::env::var("GLOBED_LESS_LOG").unwrap_or("0".to_string()) == "1" {
        log::set_max_level(LogLevelFilter::Warn);
    } else {
        log::set_max_level(if cfg!(debug_assertions) {
            LogLevelFilter::Trace
        } else {
            LogLevelFilter::Info
        });
    }

    // config file

    let mut config_path =
        std::env::var("GLOBED_CONFIG_PATH").map_or_else(|_| std::env::current_dir().unwrap(), PathBuf::from);

    if config_path.is_dir() {
        config_path = config_path.join("central-conf.json");
    }

    let config = if config_path.exists() && config_path.is_file() {
        match ServerConfig::load(&config_path) {
            Ok(x) => x,
            Err(err) => {
                error!("failed to open/parse configuration file: {err}");
                warn!("hint: if you don't have anything important there, delete the file for a new template to be created.");
                warn!("hint: the faulty configuration resides at: {config_path:?}");
                abort_misconfig();
            }
        }
    } else {
        info!("Configuration file does not exist by given path, creating a template one.");
        ServerConfig::default()
    };

    config.save(&config_path)?;

    // stupid rust

    let mnt_addr = config.web_address.clone();
    let mnt_point = config.web_mountpoint.clone();

    let state_skey = config.secret_key.clone();
    let state_skey2 = config.secret_key2.clone();

    let state = ServerState::new(ServerStateData::new(config_path.clone(), config, &state_skey, &state_skey2));

    // config file watcher

    let (mut debouncer, mut file_events) =
        AsyncDebouncer::new_with_channel(Duration::from_secs(1), Some(Duration::from_secs(1))).await?;

    debouncer.watcher().watch(&config_path, RecursiveMode::NonRecursive)?;

    let watcher_state = state.clone();
    tokio::spawn(async move {
        while let Some(_event) = file_events.recv().await {
            let mut state = watcher_state.state_write().await;
            let cpath = state.config_path.clone();
            match state.config.reload_in_place(&cpath) {
                Ok(()) => {
                    info!("Successfully reloaded the configuration");
                    // set the maintenance flag appropriately
                    watcher_state.set_maintenance(state.config.maintenance);
                }
                Err(err) => {
                    warn!("Failed to reload configuration: {}", err.to_string());
                }
            }
        }
    });

    // roa your favorite web app

    let router = web::routes::build_router();
    let route_list = router.routes(Box::leak(Box::new(mnt_point)))?;
    let app = App::state(state).end(route_list);

    let listen_addr = match mnt_addr.parse::<SocketAddr>() {
        Ok(x) => x,
        Err(err) => {
            // try parse it as an IP address and use port 41000
            if let Ok(x) = mnt_addr.parse::<IpAddr>() {
                SocketAddr::new(x, 41000)
            } else {
                error!("failed to parse the HTTP bind address: {err}");
                warn!("hint: the address must be a valid IPv4/IPv6 address with an optional port number");
                warn!("hint: examples include \"127.0.0.1\", \"0.0.0.0:41000\"");
                abort_misconfig();
            }
        }
    };

    let server = app.listen(listen_addr, |addr| {
        info!("Globed central server launched on {addr}");
    });

    let server = match server {
        Ok(x) => x,
        Err(e) => {
            error!("failed to setup the HTTP server with address {listen_addr}: {e}");
            if listen_addr.port() < 1024 {
                warn!("hint: ports below 1024 are commonly privileged and you can't use them as a regular user");
                warn!("hint: pick a higher port number or leave it out completely to use the default port number (41000)");
            }
            abort_misconfig();
        }
    };

    server.await?;

    Ok(())
}
