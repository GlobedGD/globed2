use std::net::IpAddr;

use rocket::{
    Request,
    http::Status,
    request::{FromRequest, Outcome},
};

pub mod check_protocol_;
pub(crate) use check_protocol_::check_protocol;

pub mod client_user_agent;
pub use client_user_agent::ClientUserAgentGuard;

pub mod cloudflare_ip;
pub use cloudflare_ip::CloudflareIPGuard;

pub mod game_server_password;
pub use game_server_password::GameServerPasswordGuard;

pub mod game_server_user_agent;
pub use game_server_user_agent::GameServerUserAgentGuard;

pub mod decodable;
pub use decodable::{CheckedDecodableGuard, DecodableGuard};

pub mod encrypted_json;
pub use encrypted_json::EncryptedJsonGuard;
