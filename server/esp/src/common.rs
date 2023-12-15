/* Encodable/Decodable implementations for common types */

use crate::*;
use std::{
    collections::HashMap,
    hash::{BuildHasher, Hash},
    net::{Ipv4Addr, SocketAddrV4},
    ops::Deref,
};

macro_rules! impl_primitive {
    ($typ:ty,$read:ident,$write:ident) => {
        impl crate::Encodable for $typ {
            #[inline(always)]
            fn encode(&self, buf: &mut bytebuffer::ByteBuffer) {
                buf.$write(*self);
            }

            #[inline(always)]
            fn encode_fast(&self, buf: &mut crate::FastByteBuffer) {
                buf.$write(*self);
            }
        }

        impl crate::Decodable for $typ {
            #[inline(always)]
            fn decode(buf: &mut bytebuffer::ByteBuffer) -> crate::DecodeResult<Self> {
                buf.$read().map_err(|e| e.into())
            }

            #[inline(always)]
            fn decode_from_reader(buf: &mut bytebuffer::ByteReader) -> crate::DecodeResult<Self> {
                buf.$read().map_err(|e| e.into())
            }
        }

        impl crate::KnownSize for $typ {
            const ENCODED_SIZE: usize = std::mem::size_of::<$typ>();
        }
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

encode_impl!(str, buf, self, buf.write_string(self));

/* Option<T> */

impl<T> Encodable for Option<T>
where
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_optional_value(self.as_ref());
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_optional_value(self.as_ref());
    }
}

impl<T> Decodable for Option<T>
where
    T: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_optional_value()
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_optional_value()
    }
}

impl<T> KnownSize for Option<T>
where
    T: KnownSize,
{
    const ENCODED_SIZE: usize = size_of_types!(bool, T);
}

/* [T; N] */

impl<T, const N: usize> Encodable for [T; N]
where
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_array(self);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_array(self);
    }
}

impl<T, const N: usize> Decodable for [T; N]
where
    T: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_value_array()
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_value_array()
    }
}

impl<T, const N: usize> KnownSize for [T; N]
where
    T: KnownSize,
{
    const ENCODED_SIZE: usize = size_of_types!(T) * N;
}

/* Vec<T> */

impl<T> Encodable for Vec<T>
where
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_vec(self);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_vec(self);
    }
}

impl<T> Decodable for Vec<T>
where
    T: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_value_vec()
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_value_vec()
    }
}

/* HashMap<K, V, [S]> */

impl<K, V, S: BuildHasher> Encodable for HashMap<K, V, S>
where
    K: Encodable,
    V: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_u32(self.len() as u32);
        for (k, v) in self {
            buf.write_value(k);
            buf.write_value(v);
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_u32(self.len() as u32);
        for (k, v) in self {
            buf.write_value(k);
            buf.write_value(v);
        }
    }
}

impl<K, V, S: BuildHasher + Default> Decodable for HashMap<K, V, S>
where
    K: Decodable + Hash + Eq + PartialEq,
    V: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Self::decode_from_reader(&mut ByteReader::from_bytes(buf.as_bytes()))
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let entries = buf.read_u32()?;
        let mut map = Self::default();

        for _ in 0..entries {
            let key = buf.read_value()?;
            let val = buf.read_value()?;
            map.insert(key, val);
        }

        Ok(map)
    }
}

/* RemainderBytes - wrapper around Box<[u8]> that decodes with `buf.read_remaining_bytes()` and encodes with `buf.write_bytes()` */

#[derive(Clone)]
#[repr(transparent)]
pub struct RemainderBytes {
    data: Box<[u8]>,
}

encode_impl!(RemainderBytes, buf, self, {
    buf.write_bytes(&self.data);
});

decode_impl!(RemainderBytes, buf, {
    Ok(Self {
        data: buf.read_remaining_bytes()?.into(),
    })
});

impl Deref for RemainderBytes {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl From<Vec<u8>> for RemainderBytes {
    fn from(value: Vec<u8>) -> Self {
        Self {
            data: value.into_boxed_slice(),
        }
    }
}

impl From<Box<[u8]>> for RemainderBytes {
    fn from(value: Box<[u8]>) -> Self {
        Self { data: value }
    }
}

/* Ipv4Addr */

encode_impl!(Ipv4Addr, buf, self, {
    let octets = self.octets();
    buf.write_u8(octets[0]);
    buf.write_u8(octets[1]);
    buf.write_u8(octets[2]);
    buf.write_u8(octets[3]);
});

decode_impl!(Ipv4Addr, buf, {
    let octets = [buf.read_u8()?, buf.read_u8()?, buf.read_u8()?, buf.read_u8()?];
    Ok(Self::from(octets))
});

size_calc_impl!(Ipv4Addr, size_of_types!(u8) * 4);

/* SocketAddrV4 */

encode_impl!(SocketAddrV4, buf, self, {
    buf.write_value(self.ip());
    buf.write_u16(self.port());
});

decode_impl!(SocketAddrV4, buf, {
    let ip = buf.read_value()?;
    Ok(Self::new(ip, buf.read_u16()?))
});

size_calc_impl!(SocketAddrV4, size_of_types!(Ipv4Addr, u16));
