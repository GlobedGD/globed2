/*
* Hey there!
* If you have to read this - first and foremost, I'm sorry.
* Everything you see here is the exact definition of over-engineered and over-optimized.
* Good luck.
*/

#![feature(sync_unsafe_cell)]
#![allow(
    clippy::must_use_candidate,
    clippy::module_name_repetitions,
    clippy::cast_possible_truncation,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::wildcard_imports,
    clippy::missing_safety_doc
)]

use std::{
    collections::HashMap,
    error::Error,
    net::{IpAddr, Ipv4Addr, SocketAddr},
};

use globed_shared::*;
use reqwest::StatusCode;
use server::GameServerConfiguration;
use state::ServerState;
use tokio::net::UdpSocket;

use server::GameServer;

pub mod data;
pub mod managers;
pub mod server;
pub mod server_thread;
pub mod state;
pub mod util;

struct StartupConfiguration {
    bind_address: SocketAddr,
    central_data: Option<(String, String)>,
}

fn abort_misconfig() -> ! {
    error!("aborting launch due to misconfiguration.");
    std::process::exit(1);
}

fn parse_configuration() -> StartupConfiguration {
    let mut args = std::env::args();
    let exe_name = args.next().unwrap(); // skip executable
    let arg = args.next();

    let env_addr = std::env::var("GLOBED_GS_ADDRESS");
    let using_env_variables: bool = env_addr.is_ok();

    if arg.is_none() && !using_env_variables {
        // standalone with default params
        return StartupConfiguration {
            bind_address: "0.0.0.0:41001".parse().unwrap(),
            central_data: None,
        };
    }

    // env variable takes precedence, otherwise grab the 1st arg from the command line
    let bind_address = env_addr.ok().or(arg).unwrap();

    let bind_address = match bind_address.parse::<SocketAddr>() {
        Ok(x) => x,
        Err(_) => {
            // try to parse it as an ip addr and use a default port
            match bind_address.parse::<Ipv4Addr>() {
                Ok(x) => SocketAddr::new(IpAddr::V4(x), 41001),
                Err(e) => {
                    error!("failed to parse the given IP address ({bind_address}): {e}");
                    warn!("hint: you have to provide a valid IPv4 address with an optional port number");
                    warn!("hint: for example \"0.0.0.0\" or \"0.0.0.0:41001\"");
                    abort_misconfig();
                }
            }
        }
    };

    let arg = if using_env_variables {
        std::env::var("GLOBED_GS_CENTRAL_URL").ok()
    } else {
        args.next()
    };

    if arg.is_none() {
        // standalone with a specified bind addr
        return StartupConfiguration {
            bind_address,
            central_data: None,
        };
    }

    let mut central_url = arg.unwrap();
    if !central_url.ends_with('/') {
        central_url += "/";
    }

    let arg = if using_env_variables {
        std::env::var("GLOBED_GS_CENTRAL_PASSWORD").ok()
    } else {
        args.next()
    };

    if arg.is_none() {
        if using_env_variables {
            error!("expected the environment variable 'GLOBED_GS_CENTRAL_PASSWORD', couldn't find it");
        } else {
            error!("not enough arguments, expected the password of the central server");
            error!("correct usage: \"{exe_name} <address> <central-url> <central-password>\"");
        }
        warn!("hint: you must specify the password for connecting to the central server, see the server readme.");
        abort_misconfig();
    }

    let central_pw = arg.unwrap();

    // full configuration with a central server
    StartupConfiguration {
        bind_address,
        central_data: Some((central_url, central_pw)),
    }
}

#[allow(clippy::too_many_lines)]
#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(Logger::instance("globed_game_server")).unwrap();

    if std::env::var("GLOBED_GS_LESS_LOG").unwrap_or("0".to_string()) == "1" {
        log::set_max_level(LogLevelFilter::Warn);
    } else {
        log::set_max_level(if cfg!(debug_assertions) {
            LogLevelFilter::Trace
        } else {
            LogLevelFilter::Info
        });
    }

    let startup_config = parse_configuration();
    let standalone = startup_config.central_data.is_none();

    let client = reqwest::Client::builder()
        .user_agent(format!("globed-game-server/{}", env!("CARGO_PKG_VERSION")))
        .build()
        .unwrap();

    let mut config = GameServerConfiguration {
        http_client: client,
        central_url: String::new(),
        central_pw: String::new(),
    };

    let state = ServerState::new();
    let gsbd = if standalone {
        warn!("Starting in standalone mode, authentication is disabled");
        GameServerBootData {
            protocol: PROTOCOL_VERSION,
            no_chat: Vec::new(),
            special_users: HashMap::new(),
            tps: 30,
        }
    } else {
        let (central_url, central_pw) = startup_config.central_data.unwrap();
        config.central_url = central_url;
        config.central_pw = central_pw;

        info!("Retrieving config from the central server..");

        let response = match config
            .http_client
            .post(format!("{}{}", config.central_url, "gs/boot"))
            .query(&[("pw", config.central_pw.clone())])
            .send()
            .await
        {
            Ok(x) => match x.error_for_status() {
                Ok(x) => x,
                Err(err) => {
                    error!("the central server returned an error: {err}");
                    if err.status().unwrap_or(StatusCode::OK) == StatusCode::UNAUTHORIZED {
                        warn!("hint: there is a high chance that you have supplied a wrong password");
                        warn!("hint: see the server readme if you don't know what password you need to use");
                    }
                    abort_misconfig();
                }
            },
            Err(err) => {
                error!("failed to make a request to the central server: {err}");
                warn!("hint: make sure the URL you passed is a valid Globed central server URL.");
                abort_misconfig();
            }
        };

        let configuration = response.text().await?;
        let boot_data: GameServerBootData = match serde_json::from_str(&configuration) {
            Ok(x) => x,
            Err(err) => {
                error!("failed to parse the data sent by the central server: {err}");
                warn!("hint: make sure the URL you passed is a valid Globed central server URL.");
                abort_misconfig();
            }
        };

        if boot_data.protocol != PROTOCOL_VERSION {
            error!("incompatible protocol versions!");
            error!(
                "this game server is on v{PROTOCOL_VERSION}, while the central server uses v{}",
                boot_data.protocol
            );
            if boot_data.protocol > PROTOCOL_VERSION {
                warn!(
                    "hint: you are running an old version of the Globed game server (v{}), please update to the latest one.",
                    env!("CARGO_PKG_VERSION")
                );
            } else {
                warn!("hint: the central server you are using is outdated, or the game server is using a development build that is too new.");
            }
            abort_misconfig();
        }

        boot_data
    };

    let socket = match UdpSocket::bind(&startup_config.bind_address).await {
        Ok(x) => x,
        Err(err) => {
            error!(
                "Failed to bind the socket with address {}: {err}",
                startup_config.bind_address
            );
            if startup_config.bind_address.port() < 1024 {
                warn!("hint: ports below 1024 are commonly privileged and you can't use them as a regular user");
                warn!("hint: pick a higher port number or use port 0 to get a randomly generated port");
            }
            abort_misconfig();
        }
    };

    let server = GameServer::new(socket, state, gsbd, config, standalone);
    let server = Box::leak(Box::new(server));

    Box::pin(server.run()).await;

    Ok(())
}
