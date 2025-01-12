use super::*;

use globed_shared::esp::{ByteBuffer, ByteBufferExtRead, Decodable, DecodeError};
use rocket::{
    Data, Request,
    data::{self, FromData, ToByteUnit},
};
use tokio::io::AsyncReadExt;

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
