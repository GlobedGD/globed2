use anyhow::Result;
use std::io::Read;

use bytebuffer::{ByteBuffer, ByteReader};
pub use crypto_box::{PublicKey, KEY_SIZE};

use crate::data::bytebufferext::*;

/* Encodable/Decodable implementations for common types */

macro_rules! impl_primitive {
    ($typ:ty,$read:ident,$write:ident) => {
        encode_impl!($typ, buf, self, {
            buf.$write(*self);
        });

        decode_impl!($typ, buf, { buf.$read().map_err(|e| e.into()) });

        size_calc_impl!($typ, size_of_primitives!(Self));
    };
}

impl_primitive!(bool, read_bool, write_bool);
impl_primitive!(u8, read_u8, write_u8);
impl_primitive!(u16, read_u16, write_u16);
impl_primitive!(u32, read_u32, write_u32);
impl_primitive!(u64, read_u64, write_u64);
impl_primitive!(i8, read_i8, write_i8);
impl_primitive!(i16, read_i16, write_i16);
impl_primitive!(i32, read_i32, write_i32);
impl_primitive!(i64, read_i64, write_i64);
impl_primitive!(f32, read_f32, write_f32);
impl_primitive!(f64, read_f64, write_f64);

encode_impl!(String, buf, self, buf.write_string(self));
decode_impl!(String, buf, Ok(buf.read_string()?));

encode_impl!(&str, buf, self, buf.write_string(self));

/* Option<T> */

impl<T> Encodable for Option<T>
where
    T: Encodable,
{
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_optional_value(self.as_ref());
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_optional_value(self.as_ref());
    }
}

impl<T> EncodableWithKnownSize for Option<T>
where
    T: EncodableWithKnownSize,
{
    const ENCODED_SIZE: usize = size_of_types!(bool, T);
}

impl<T> Decodable for Option<T>
where
    T: Decodable,
{
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_optional_value()
    }

    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_optional_value()
    }
}

/* [T; N] */

impl<T, const N: usize> Encodable for [T; N]
where
    T: Encodable,
{
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_array(self);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_array(self);
    }
}

impl<T, const N: usize> EncodableWithKnownSize for [T; N]
where
    T: EncodableWithKnownSize,
{
    const ENCODED_SIZE: usize = size_of_types!(T) * N;
}

impl<T, const N: usize> Decodable for [T; N]
where
    T: Decodable,
{
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_value_array()
    }

    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_value_array()
    }
}

/* Vec<T> */

impl<T> Encodable for Vec<T>
where
    T: Encodable,
{
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_vec(self);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_vec(self);
    }
}

impl<T> Decodable for Vec<T>
where
    T: Decodable,
{
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_value_vec()
    }

    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_value_vec()
    }
}

/* crypto_box::PublicKey */

impl Encodable for PublicKey {
    fn encode(&self, buf: &mut bytebuffer::ByteBuffer) {
        buf.write_bytes(self.as_bytes());
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_bytes(self.as_bytes());
    }
}

impl EncodableWithKnownSize for PublicKey {
    const ENCODED_SIZE: usize = KEY_SIZE;
}

impl Decodable for PublicKey {
    fn decode(buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        Self::decode_from_reader(&mut ByteReader::from_bytes(buf.as_bytes()))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        let mut key = [0u8; KEY_SIZE];
        buf.read_exact(&mut key)?;

        Ok(Self::from_bytes(key))
    }
}
