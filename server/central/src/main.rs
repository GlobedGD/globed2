#![feature(duration_constructors)]
#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::wildcard_imports,
    clippy::no_effect_underscore_binding,
    clippy::redundant_closure_for_method_calls
)]

use std::{
    error::Error,
    path::{Path, PathBuf},
    time::Duration,
};

use async_watcher::{notify::RecursiveMode, AsyncDebouncer};
use config::ServerConfig;
use db::GlobedDb;
use game_pinger::GameServerPinger;
use globed_shared::{
    get_log_level,
    logger::{error, info, log, warn, Logger},
    LogLevelFilter,
};
use rocket::catchers;
use rocket_db_pools::Database;
use state::{ServerState, ServerStateData};
use tokio::io::AsyncWriteExt;

pub mod config;
pub mod db;
pub mod game_pinger;
pub mod ip_blocker;
pub mod state;
pub mod verifier;
pub mod web;

fn abort_misconfig() -> ! {
    error!("aborting launch due to misconfiguration.");
    std::process::exit(1);
}

#[rocket::main]
#[allow(clippy::too_many_lines)]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(Logger::instance("globed_central_server", false)).unwrap();

    if let Some(log_level) = get_log_level("GLOBED_LOG_LEVEL") {
        log::set_max_level(log_level);
    } else {
        log::set_max_level(LogLevelFilter::Warn); // we have to print these logs somehow lol
        error!("invalid value for the log level environment varaible");
        warn!("hint: possible values are 'trace', 'debug', 'info', 'warn', 'error', and 'none'.");
        abort_misconfig();
    }

    // create Rocket.toml if it doesn't exist
    let rocket_toml =
        std::env::var("ROCKET_CONFIG").map_or_else(|_| std::env::current_dir().unwrap().join("Rocket.toml"), PathBuf::from);

    if !rocket_toml.file_name().is_some_and(|x| x == "Rocket.toml") || !rocket_toml.parent().is_some_and(Path::exists) {
        error!("invalid value for ROCKET_CONFIG");
        warn!("hint: the filename must be 'Rocket.toml' and the parent folder must exist on the disk");
        abort_misconfig();
    }

    if !rocket_toml.exists() {
        info!("Creating a template Rocket.toml file");
        let mut file = tokio::fs::File::create(rocket_toml).await?;
        file.write_all(include_bytes!("Rocket.toml.template")).await?;
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

        let conf = ServerConfig::default();
        conf.save(&config_path)?;

        conf
    };

    // stupid rust

    let mnt_point = config.web_mountpoint.clone();

    let state_skey = config.secret_key.clone();
    let state_skey2 = config.secret_key2.clone();

    let pinger = GameServerPinger::new(&config.game_servers).await;
    let ssd = ServerStateData::new(config_path.clone(), config, &state_skey, &state_skey2);
    let state = ServerState::new(ssd, pinger);

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
                    watcher_state.inner.verifier.set_enabled(state.config.use_gd_api);
                }
                Err(err) => {
                    warn!("Failed to reload configuration: {}", err.to_string());
                }
            }
        }
    });

    // account verification stuff
    let av_state = state.inner.clone();
    let av_state2 = state.inner.clone();
    tokio::spawn(async move {
        av_state.verifier.run_refresher().await;
    });

    tokio::spawn(async move {
        av_state2.verifier.run_deleter().await;
    });

    // woo
    let pinger_state = state.inner.clone();
    tokio::spawn(async move {
        pinger_state.pinger.run_pinger().await;
    });

    // start up rocket

    let rocket = rocket::build()
        .register("/", catchers![web::not_found, web::query_string])
        .mount(mnt_point, web::routes::build_router())
        .manage(state)
        .manage(
            if std::env::var("GLOBED_DISABLE_CORS").unwrap_or_else(|_| "0".to_owned()) == "0" {
                // keep cors enabled
                rocket_cors::CorsOptions::default()
                    .allowed_origins(rocket_cors::AllowedOrigins::all())
                    .allowed_methods(vec![].into_iter().collect())
                    .allow_credentials(false)
                    .to_cors()?
            } else {
                // disable cors for GET methods
                warn!("disabling CORS for get requests");
                rocket_cors::CorsOptions::default()
                    .allowed_origins(rocket_cors::AllowedOrigins::all())
                    .allowed_methods(vec![rocket::http::Method::Get].into_iter().map(From::from).collect())
                    .allow_credentials(false)
                    .to_cors()?
            },
        )
        .attach(GlobedDb::init())
        .attach(db::migration_fairing());

    rocket.launch().await?;

    Ok(())
}
