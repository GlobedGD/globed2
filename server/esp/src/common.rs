/* Encodable/Decodable implementations for common types */

use crate::*;
use std::{
    collections::HashMap,
    hash::{BuildHasher, Hash},
    net::{Ipv4Addr, SocketAddrV4},
    ops::Deref,
};

const fn constmax(a: usize, b: usize) -> usize {
    if a > b {
        a
    } else {
        b
    }
}

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

        impl crate::StaticSize for $typ {
            const ENCODED_SIZE: usize = std::mem::size_of::<$typ>();
        }

        impl crate::DynamicSize for $typ {
            #[inline(always)]
            fn encoded_size(&self) -> usize {
                Self::ENCODED_SIZE
            }
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

/* strings */

encode_impl!(String, buf, self, buf.write_string(self));
decode_impl!(String, buf, Ok(buf.read_string()?));
dynamic_size_calc_impl!(String, self, size_of_types!(u32) + self.len());

encode_impl!(str, buf, self, buf.write_string(self));
dynamic_size_calc_impl!(str, self, size_of_types!(u32) + self.len());

/* references (yes, that is really needed) */

impl<T> Encodable for &T
where
    T: Encodable + ?Sized,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        T::encode(self, buf);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        T::encode_fast(self, buf);
    }
}

impl<T> StaticSize for &T
where
    T: StaticSize + ?Sized,
{
    const ENCODED_SIZE: usize = T::ENCODED_SIZE;
}

impl<T> DynamicSize for &T
where
    T: DynamicSize + ?Sized,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        T::encoded_size(self)
    }
}

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

impl<T> StaticSize for Option<T>
where
    T: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(bool, T);
}

impl<T> DynamicSize for Option<T>
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(bool) + self.as_ref().map_or(0, T::encoded_size)
    }
}

/* Result<T, E> */

impl<T, E> Encodable for Result<T, E>
where
    T: Encodable,
    E: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_bool(self.is_ok());
        match self {
            Ok(x) => buf.write_value(x),
            Err(e) => buf.write_value(e),
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_bool(self.is_ok());
        match self {
            Ok(x) => buf.write_value(x),
            Err(e) => buf.write_value(e),
        }
    }
}

impl<T, E> Decodable for Result<T, E>
where
    T: Decodable,
    E: Decodable,
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
        Ok(if buf.read_bool()? {
            Ok(buf.read_value::<T>()?)
        } else {
            Err(buf.read_value::<E>()?)
        })
    }
}

// not sure if this one makes any sense
impl<T, E> StaticSize for Result<T, E>
where
    T: StaticSize,
    E: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(bool) + constmax(T::ENCODED_SIZE, E::ENCODED_SIZE);
}

impl<T, E> DynamicSize for Result<T, E>
where
    T: DynamicSize,
    E: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        size_of_types!(bool)
            + match self.as_ref() {
                Ok(x) => x.encoded_size(),
                Err(e) => e.encoded_size(),
            }
    }
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

impl<T, const N: usize> StaticSize for [T; N]
where
    T: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(T) * N;
}

impl<T, const N: usize> DynamicSize for [T; N]
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        self.iter().map(T::encoded_size).sum()
    }
}

/* [T] */

impl<T> Encodable for [T]
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

impl<T> DynamicSize for [T]
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.iter().map(T::encoded_size).sum::<usize>()
    }
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

impl<T> DynamicSize for Vec<T>
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.iter().map(T::encoded_size).sum::<usize>()
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

impl<K, V, S> DynamicSize for HashMap<K, V, S>
where
    K: DynamicSize,
    V: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.iter().map(|(k, v)| k.encoded_size() + v.encoded_size()).sum::<usize>()
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

dynamic_size_calc_impl!(RemainderBytes, self, self.data.len());

impl Deref for RemainderBytes {
    type Target = [u8];
    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl From<Vec<u8>> for RemainderBytes {
    #[inline]
    fn from(value: Vec<u8>) -> Self {
        Self {
            data: value.into_boxed_slice(),
        }
    }
}

impl From<Box<[u8]>> for RemainderBytes {
    #[inline]
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

static_size_calc_impl!(Ipv4Addr, size_of_types!(u8) * 4);
dynamic_size_as_static_impl!(Ipv4Addr);

/* SocketAddrV4 */

encode_impl!(SocketAddrV4, buf, self, {
    buf.write_value(self.ip());
    buf.write_u16(self.port());
});

decode_impl!(SocketAddrV4, buf, {
    let ip = buf.read_value()?;
    Ok(Self::new(ip, buf.read_u16()?))
});

static_size_calc_impl!(SocketAddrV4, size_of_types!(Ipv4Addr, u16));
dynamic_size_as_static_impl!(SocketAddrV4);
