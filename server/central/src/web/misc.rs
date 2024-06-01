use std::{error::Error, io::Cursor, net::IpAddr};

use globed_shared::esp::{ByteBuffer, ByteBufferExtRead, ByteBufferExtWrite, Decodable, DecodeError, Encodable};
use rocket::{
    data::{self, FromData, ToByteUnit},
    http::{ContentType, Status},
    request::{FromRequest, Outcome},
    response::{self, Responder},
    Data, Request, Response,
};
use tokio::io::AsyncReadExt;

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
    pub fn new(inner: &str) -> Self {
        Self { inner: inner.to_owned() }
    }
}

macro_rules! unauthorized {
    ($msg:expr) => {
        return Err(UnauthorizedResponder::new($msg).into())
    };
}

pub(crate) use unauthorized;

#[derive(Responder)]
#[response(status = 400, content_type = "text")]
pub struct BadRequestResponder {
    pub inner: String,
}

impl BadRequestResponder {
    pub fn new(inner: &str) -> Self {
        Self { inner: inner.to_owned() }
    }
}

macro_rules! bad_request {
    ($msg:expr) => {
        return Err(BadRequestResponder::new($msg).into())
    };
}

macro_rules! check_protocol {
    ($protocol:expr) => {
        let p = $protocol;
        if p != PROTOCOL_VERSION && p != 0xffff {
            bad_request!(&format!(
                "Outdated client, please update Globed. This server requires at least version {MIN_CLIENT_VERSION}.",
            ));
        }
    };
}

pub(crate) use bad_request;
pub(crate) use check_protocol;

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

impl From<BadRequestResponder> for GenericErrorResponder<String> {
    fn from(value: BadRequestResponder) -> Self {
        GenericErrorResponder {
            inner: (Status::BadRequest, value.inner),
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

pub type WebResult<T> = Result<T, GenericErrorResponder<String>>;

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

// Responder wrapper type for types that implement `esp::Encodable`.

pub struct EncodableResponder {
    data: Box<[u8]>,
}

impl EncodableResponder {
    pub fn new<T: Encodable>(value: T) -> Self {
        let mut buffer = ByteBuffer::new();
        value.encode(&mut buffer);
        Self {
            data: buffer.into_vec().into_boxed_slice(),
        }
    }
}

#[rocket::async_trait]
impl<'r> Responder<'r, 'static> for EncodableResponder {
    fn respond_to(self, _: &'r Request<'_>) -> response::Result<'static> {
        Response::build()
            .header(ContentType::Binary)
            .sized_body(self.data.len(), Cursor::new(self.data))
            .ok()
    }
}

// same as `EncodableResponder` but appends the checksum at the end of the response

pub struct CheckedEncodableResponder {
    data: Box<[u8]>,
}

impl CheckedEncodableResponder {
    pub fn new<T: Encodable>(value: T) -> Self {
        let mut buffer = ByteBuffer::new();
        buffer.write_value(&value);
        buffer.append_self_checksum();

        Self {
            data: buffer.into_vec().into_boxed_slice(),
        }
    }
}

#[rocket::async_trait]
impl<'r> Responder<'r, 'static> for CheckedEncodableResponder {
    fn respond_to(self, _: &'r Request<'_>) -> response::Result<'static> {
        Response::build()
            .header(ContentType::Binary)
            .sized_body(self.data.len(), Cursor::new(self.data))
            .ok()
    }
}

// Request guard for types that implement `esp::Decodable`.

pub struct DecodableGuard<T: Decodable>(pub T);

#[rocket::async_trait]
impl<'r, T: Decodable> FromData<'r> for DecodableGuard<T> {
    type Error = DecodeError;

    async fn from_data(_req: &'r Request<'_>, data: Data<'r>) -> data::Outcome<'r, Self> {
        let mut stream = data.open(1.mebibytes());

        let mut vec = Vec::new();
        match stream.read_to_end(&mut vec).await {
            Ok(_) => {}
            Err(_) => return data::Outcome::Error((Status::BadRequest, DecodeError::NotEnoughData)),
        }

        let mut buffer = ByteBuffer::from_vec(vec);
        match buffer.read_value() {
            Ok(value) => data::Outcome::Success(Self(value)),
            Err(err) => data::Outcome::Error((Status::BadRequest, err)),
        }
    }
}

// Same as `DecodableGuard` but reads a checksum at the end of the response and validates it

pub struct CheckedDecodableGuard<T: Decodable>(pub T);

#[rocket::async_trait]
impl<'r, T: Decodable> FromData<'r> for CheckedDecodableGuard<T> {
    type Error = DecodeError;

    async fn from_data(_req: &'r Request<'_>, data: Data<'r>) -> data::Outcome<'r, Self> {
        let mut stream = data.open(1.mebibytes());

        let mut vec = Vec::new();
        match stream.read_to_end(&mut vec).await {
            Ok(_) => {}
            Err(_) => return data::Outcome::Error((Status::BadRequest, DecodeError::NotEnoughData)),
        }

        let mut buffer = ByteBuffer::from_vec(vec);
        match buffer.validate_self_checksum() {
            Ok(()) => {}
            Err(err) => return data::Outcome::Error((Status::BadRequest, err)),
        }

        match buffer.read_value() {
            Ok(value) => data::Outcome::Success(Self(value)),
            Err(err) => data::Outcome::Error((Status::BadRequest, err)),
        }
    }
}

// game server password guar
pub struct GameServerPasswordGuard(pub String);

#[rocket::async_trait]
impl<'r> FromRequest<'r> for GameServerPasswordGuard {
    type Error = &'static str;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        match request.headers().get_one("Authorization") {
            Some(x) => Outcome::Success(GameServerPasswordGuard(x.to_owned())),
            None => Outcome::Error((Status::Unauthorized, "no password provided")),
        }
    }
}

impl GameServerPasswordGuard {
    pub fn verify(&self, correct: &str) -> bool {
        if self.0.len() != correct.len() {
            return false;
        }

        let bytes1 = self.0.as_bytes();
        let bytes2 = correct.as_bytes();

        let mut result = 0u8;
        for (b1, b2) in bytes1.iter().zip(bytes2.iter()) {
            result |= b1 ^ b2;
        }

        result == 0
    }
}
