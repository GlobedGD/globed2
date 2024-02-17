use std::{error::Error, net::IpAddr};

use rocket::{
    http::Status,
    request::{FromRequest, Outcome},
    Request, Responder,
};

#[derive(Responder)]
#[response(status = 503, content_type = "text")]
pub struct MaintenanceResponder {
    pub inner: &'static str,
}

#[derive(Responder)]
#[response(status = 401, content_type = "text")]
pub struct UnauthorizedResponder {
    pub inner: String,
}

impl UnauthorizedResponder {
    pub fn new(inner: String) -> Self {
        Self { inner }
    }
}

#[derive(Responder)]
pub struct GenericErrorResponder<T> {
    pub inner: (Status, T),
}

impl From<MaintenanceResponder> for GenericErrorResponder<String> {
    fn from(value: MaintenanceResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::ServiceUnavailable, value.inner.to_string()),
        }
    }
}

impl From<UnauthorizedResponder> for GenericErrorResponder<String> {
    fn from(value: UnauthorizedResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::Unauthorized, value.inner),
        }
    }
}

impl<T> From<T> for GenericErrorResponder<String>
where
    T: Error,
{
    fn from(value: T) -> Self {
        GenericErrorResponder {
            inner: (Status::InternalServerError, value.to_string()),
        }
    }
}

// game server user agent
pub struct GameServerUserAgentGuard<'r>(pub &'r str);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for GameServerUserAgentGuard<'r> {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("User-Agent") {
            Some(ua) if ua.starts_with("globed-game-server") => Outcome::Success(GameServerUserAgentGuard(ua)),
            Some(_) => Outcome::Error((Status::Unauthorized, "bad request?")),
            None => Outcome::Error((Status::Unauthorized, "missing useragent")),
        }
    }
}

// client user agent
pub struct ClientUserAgentGuard<'r>(pub &'r str);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for ClientUserAgentGuard<'r> {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("User-Agent") {
            Some(ua) if ua.starts_with("globed-geode-xd") => Outcome::Success(ClientUserAgentGuard(ua)),
            Some(_) => Outcome::Error((Status::Unauthorized, "bad request?")),
            None => Outcome::Error((Status::Unauthorized, "missing useragent")),
        }
    }
}

// client ip address
pub struct CloudflareIPGuard(pub Option<IpAddr>);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for CloudflareIPGuard {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("CF-Connecting-IP") {
            Some(x) => match x.parse::<IpAddr>() {
                Ok(a) => Outcome::Success(CloudflareIPGuard(Some(a))),
                Err(_) => Outcome::Error((Status::Unauthorized, "failed to parse the IP header from Cloudflare")),
            },
            None => Outcome::Success(CloudflareIPGuard(None)),
        }
    }
}
