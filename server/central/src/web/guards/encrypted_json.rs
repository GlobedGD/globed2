use std::{num::ParseIntError, sync::OnceLock};

use super::*;
use globed_shared::{CryptoBox, CryptoBoxError};
use rocket::{
    Data,
    data::{self, FromData, ToByteUnit},
};
use serde::de::DeserializeOwned;
use tokio::io::AsyncReadExt;

// this isnt really meant to be secret but yeah
// the skids can have fun with this :D
const FUNNY_STRING: &str = "bff252d2731a6c6ca26d7f5144bc750fd6723316619f86c8636ebdc13bf3214c";

// Request guard for JSON data that is encrypted

pub struct EncryptedJsonGuard<T>(pub T);

#[derive(Debug)]
pub enum EncryptedJsonGuardError {
    EndOfStream,
    DecryptionError(CryptoBoxError),
    Parsing(serde_json::Error),
}

fn decode_hex(s: &str) -> Result<Vec<u8>, ParseIntError> {
    (0..s.len()).step_by(2).map(|i| u8::from_str_radix(&s[i..i + 2], 16)).collect()
}

#[rocket::async_trait]
impl<'r, T: DeserializeOwned> FromData<'r> for EncryptedJsonGuard<T> {
    type Error = EncryptedJsonGuardError;

    async fn from_data(_req: &'r Request<'_>, data: Data<'r>) -> data::Outcome<'r, Self> {
        static BOX: OnceLock<CryptoBox> = OnceLock::new();

        let mut stream = data.open(1.mebibytes());

        let mut vec = Vec::new();
        match stream.read_to_end(&mut vec).await {
            Ok(_) => {}
            Err(_) => return data::Outcome::Error((Status::BadRequest, EncryptedJsonGuardError::EndOfStream)),
        }

        // decrypt the data
        let crypto_box = BOX.get_or_init(|| CryptoBox::new_secret(&decode_hex(FUNNY_STRING).unwrap()));
        match crypto_box.decrypt_in_place(&mut vec) {
            Ok(size) => {
                vec.resize(size, 0);
            }
            Err(e) => return data::Outcome::Error((Status::BadRequest, EncryptedJsonGuardError::DecryptionError(e))),
        }

        let value: T = match serde_json::from_slice(&vec) {
            Ok(x) => x,
            Err(e) => return data::Outcome::Error((Status::BadRequest, EncryptedJsonGuardError::Parsing(e))),
        };

        data::Outcome::Success(Self(value))
    }
}
