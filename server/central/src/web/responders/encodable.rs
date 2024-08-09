use super::*;
use std::io::Cursor;

use globed_shared::esp::{ByteBuffer, ByteBufferExtWrite, Encodable};
use rocket::http::ContentType;

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
    fn respond_to(self, _: &'r Request<'_>) -> rocket::response::Result<'static> {
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
    fn respond_to(self, _: &'r Request<'_>) -> rocket::response::Result<'static> {
        Response::build()
            .header(ContentType::Binary)
            .sized_body(self.data.len(), Cursor::new(self.data))
            .ok()
    }
}
