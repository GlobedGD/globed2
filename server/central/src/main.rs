use std::{error::Error, path::PathBuf, sync::Arc, time::Duration};

use async_rate_limit::sliding_window::SlidingWindowRateLimiter;
use async_watcher::{notify::RecursiveMode, AsyncDebouncer};
use config::ServerConfig;
use log::{error, info, warn, LevelFilter};
use logger::Logger;
use roa::{tcp::Listener, App};
use state::{ServerState, ServerStateData};
use tokio::sync::RwLock;

pub mod config;
pub mod ip_blocker;
pub mod logger;
pub mod state;
pub mod web;

static LOGGER: Logger = Logger;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(&LOGGER).unwrap();

    if std::env::var("GLOBED_LESS_LOG").unwrap_or("0".to_string()) == "1" {
        log::set_max_level(LevelFilter::Warn);
    } else {
        log::set_max_level(if cfg!(debug_assertions) {
            LevelFilter::Trace
        } else {
            LevelFilter::Info
        });
    }

    // config file

    let mut config_path = std::env::var("GLOBED_CONFIG_PATH")
        .map(PathBuf::from)
        .unwrap_or_else(|_| std::env::current_dir().unwrap());

    if config_path.is_dir() {
        config_path = config_path.join("central-conf.json");
    }

    let config = if config_path.exists() && config_path.is_file() {
        match ServerConfig::load(&config_path) {
            Ok(x) => x,
            Err(err) => {
                error!("Failed to open configuration file: {}", err.to_string());
                error!("Please fix the mistakes in the file or delete it for a new template to be created.");
                panic!("aborting due to broken config");
            }
        }
    } else {
        info!("Configuration file does not exist by given path, creating a template one.");
        ServerConfig::make_default()
    };

    config.save(&config_path)?;

    // stupid rust

    let mnt_addr = config.web_address.clone();
    let mnt_point = config.web_mountpoint.clone();

    let state_skey = config.secret_key.clone();

    let state = ServerState {
        inner: Arc::new(RwLock::new(ServerStateData::new(config_path.clone(), config, state_skey))),
    };

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
                Ok(_) => {
                    info!("Successfully reloaded the configuration");
                }
                Err(err) => {
                    warn!("Failed to reload configuration: {}", err.to_string());
                }
            }
        }
    });

    // roa your favorite web app

    let router = web::routes::build_router();
    let routes = router.routes(Box::leak(Box::new(mnt_point)))?;
    let app = App::state(state).end(routes);

    app.listen(mnt_addr, |addr| {
        info!("Globed central server launched on {addr}");
    })?
    .await?;

    Ok(())
}
