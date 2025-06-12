/* Encodable/Decodable implementations for common types */

use crate::*;
use std::{
    borrow::Cow,
    collections::HashMap,
    hash::{BuildHasher, Hash},
    net::{Ipv4Addr, SocketAddrV4},
};

// this is bizarre
const fn constmax(a: usize, b: usize) -> usize {
    if a >= b { a } else { b }
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

impl Encodable for String {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_length(self.len());
        buf.write_bytes(self.as_bytes());
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_length(self.len());
        buf.write_bytes(self.as_bytes());
    }
}

impl Decodable for String {
    #[allow(clippy::string_from_utf8_as_bytes)]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let len = buf.read_length_check::<u8>()?;

        let end_pos = buf.get_rpos() + len;
        let str = std::str::from_utf8(&buf.as_bytes()[buf.get_rpos()..end_pos])
            .map_err(|_| DecodeError::InvalidStringValue)?
            .to_owned();

        buf.set_rpos(end_pos);

        Ok(str)
    }
}

dynamic_size_calc_impl!(String, self, size_of_types!(VarLength) + self.len());

impl Encodable for str {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_length(self.len());
        buf.write_bytes(self.as_bytes());
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_length(self.len());
        buf.write_bytes(self.as_bytes());
    }
}

dynamic_size_calc_impl!(str, self, size_of_types!(VarLength) + self.len());

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

/* Cow<'a, T> */

impl<T: ?Sized + ToOwned> Encodable for Cow<'_, T>
where
    <T as ToOwned>::Owned: Encodable,
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        match self {
            Cow::Borrowed(b) => b.encode(buf),
            Cow::Owned(o) => o.encode(buf),
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        match self {
            Cow::Borrowed(b) => b.encode_fast(buf),
            Cow::Owned(o) => o.encode_fast(buf),
        }
    }
}

impl<T: ?Sized + ToOwned> Decodable for Cow<'_, T>
where
    <T as ToOwned>::Owned: Decodable,
    T: Decodable,
{
    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(Cow::Owned(buf.read_value()?))
    }
}

impl<T: ?Sized + ToOwned> DynamicSize for Cow<'_, T>
where
    <T as ToOwned>::Owned: DynamicSize,
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        match self {
            Cow::Borrowed(b) => b.encoded_size(),
            Cow::Owned(o) => o.encoded_size(),
        }
    }
}

/* Option<T> */

impl<T> Encodable for Option<T>
where
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_bool(self.is_some());
        if let Some(v) = self {
            buf.write_value(v);
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_bool(self.is_some());
        if let Some(v) = self {
            buf.write_value(v);
        }
    }
}

impl<T> Decodable for Option<T>
where
    T: Decodable,
{
    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(if buf.read_bool()? { Some(buf.read_value::<T>()?) } else { None })
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
        Ok(if buf.read_bool()? {
            Ok(buf.read_value::<T>()?)
        } else {
            Err(buf.read_value::<E>()?)
        })
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
        buf.write_length(self.len());
        for elem in self {
            buf.write_value(elem);
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_length(self.len());
        for elem in self {
            buf.write_value(elem);
        }
    }
}

impl<T> DynamicSize for [T]
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(VarLength) + self.iter().map(T::encoded_size).sum::<usize>()
    }
}

/* Vec<T> */

impl<T> Encodable for Vec<T>
where
    T: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        // encode as &[T]
        (**self).encode(buf);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        // encode as &[T]
        (**self).encode_fast(buf);
    }
}

impl<T> Decodable for Vec<T>
where
    T: Decodable,
{
    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let mut out = Vec::new();
        let length = buf.read_length()?;
        for _ in 0..length {
            out.push(buf.read_value()?);
        }

        Ok(out)
    }
}

impl<T> DynamicSize for Vec<T>
where
    T: DynamicSize,
{
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(VarLength) + self.iter().map(T::encoded_size).sum::<usize>()
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
        buf.write_length(self.len());
        for (k, v) in self {
            buf.write_value(k);
            buf.write_value(v);
        }
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_length(self.len());
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
        let entries = buf.read_length()?;
        let mut map = Self::default();

        for _ in 0..entries {
            let key = buf.read_value()?;
            let val = buf.read_value()?;
            map.insert(key, val);
        }

        Ok(map)
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let entries = buf.read_length()?;
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
        size_of_types!(VarLength) + self.iter().map(|(k, v)| k.encoded_size() + v.encoded_size()).sum::<usize>()
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

/* tuples (only 2 and 3 and 4 elements because i cant figure out how to make it a macro lmao) */
/* if you feel bored, feel free to make your attempt at making a macro for this */

impl<A, B> Encodable for (A, B)
where
    A: Encodable,
    B: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
    }
}

impl<A, B> Decodable for (A, B)
where
    A: Decodable,
    B: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?))
    }
}

impl<A, B> StaticSize for (A, B)
where
    A: StaticSize,
    B: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(A, B);
}

impl<A, B> DynamicSize for (A, B)
where
    A: DynamicSize,
    B: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        self.0.encoded_size() + self.1.encoded_size()
    }
}

// 3

impl<A, B, C> Encodable for (A, B, C)
where
    A: Encodable,
    B: Encodable,
    C: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
        buf.write_value(&self.2);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
        buf.write_value(&self.2);
    }
}

impl<A, B, C> Decodable for (A, B, C)
where
    A: Decodable,
    B: Decodable,
    C: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?, buf.read_value()?))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?, buf.read_value()?))
    }
}

impl<A, B, C> StaticSize for (A, B, C)
where
    A: StaticSize,
    B: StaticSize,
    C: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(A, B, C);
}

impl<A, B, C> DynamicSize for (A, B, C)
where
    A: DynamicSize,
    B: DynamicSize,
    C: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        self.0.encoded_size() + self.1.encoded_size() + self.2.encoded_size()
    }
}

// 4

impl<A, B, C, D> Encodable for (A, B, C, D)
where
    A: Encodable,
    B: Encodable,
    C: Encodable,
    D: Encodable,
{
    #[inline]
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
        buf.write_value(&self.2);
        buf.write_value(&self.3);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value(&self.0);
        buf.write_value(&self.1);
        buf.write_value(&self.2);
        buf.write_value(&self.3);
    }
}

impl<A, B, C, D> Decodable for (A, B, C, D)
where
    A: Decodable,
    B: Decodable,
    C: Decodable,
    D: Decodable,
{
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?, buf.read_value()?, buf.read_value()?))
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok((buf.read_value()?, buf.read_value()?, buf.read_value()?, buf.read_value()?))
    }
}

impl<A, B, C, D> StaticSize for (A, B, C, D)
where
    A: StaticSize,
    B: StaticSize,
    C: StaticSize,
    D: StaticSize,
{
    const ENCODED_SIZE: usize = size_of_types!(A, B, C, D);
}

impl<A, B, C, D> DynamicSize for (A, B, C, D)
where
    A: DynamicSize,
    B: DynamicSize,
    C: DynamicSize,
    D: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        self.0.encoded_size() + self.1.encoded_size() + self.2.encoded_size() + self.3.encoded_size()
    }
}
