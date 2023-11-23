use crate::data::types::cocos;
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};

pub trait Encodable {
    fn encode(&self, buf: &mut ByteBuffer);
    fn encode_fast(&self, buf: &mut FastByteBuffer);
}

pub trait Decodable {
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized;
    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized;
}

macro_rules! encode_impl {
    ($packet_type:ty, $buf:ident, $self:ident, $encode:expr) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&$self, $buf: &mut bytebuffer::ByteBuffer) {
                $encode
            }

            fn encode_fast(&$self, $buf: &mut crate::bytebufferext::FastByteBuffer) {
                $encode
            }
        }
    };
}

macro_rules! decode_impl {
    ($packet_type:ty, $buf:ident, $decode:expr) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode($buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self> {
                $decode
            }

            fn decode_from_reader($buf: &mut bytebuffer::ByteReader) -> anyhow::Result<Self> {
                $decode
            }
        }
    };
}

macro_rules! encode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&self, _: &mut bytebuffer::ByteBuffer) {
                panic!(
                    "Tried to call {}::encode when Encodable was not implemented for this type",
                    stringify!($packet_type)
                );
            }

            fn encode_fast(&self, _: &mut crate::bytebufferext::FastByteBuffer) {
                panic!(
                    "Tried to call {}::encode_fast when Encodable was not implemented for this type",
                    stringify!($packet_type)
                );
            }
        }
    };
}

macro_rules! decode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode(_: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self> {
                Err(anyhow::anyhow!(
                    "decoding unimplemented for {}",
                    stringify!($packet_type)
                ))
            }

            fn decode_from_reader(_: &mut bytebuffer::ByteReader) -> anyhow::Result<Self> {
                Err(anyhow::anyhow!(
                    "decoding unimplemented for {}",
                    stringify!($packet_type)
                ))
            }
        }
    };
}

pub(crate) use decode_impl;
pub(crate) use decode_unimpl;
pub(crate) use encode_impl;
pub(crate) use encode_unimpl;

/* ByteBuffer extensions */
pub trait ByteBufferExt {
    fn with_capacity(capacity: usize) -> Self;
}

pub trait ByteBufferExtWrite {
    fn write_bool(&mut self, val: bool);
    // write a byte vector, prefixed with 4 bytes indicating length
    fn write_byte_array(&mut self, vec: &[u8]);

    fn write_value<T: Encodable>(&mut self, val: &T);
    fn write_optional_value<T: Encodable>(&mut self, val: Option<&T>);

    fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]);
    fn write_value_vec<T: Encodable>(&mut self, val: &[T]);

    fn write_enum<E: Into<B>, B: Encodable>(&mut self, val: E);

    fn write_color3(&mut self, val: cocos::Color3B);
    fn write_color4(&mut self, val: cocos::Color4B);
    fn write_point(&mut self, val: cocos::Point);
}

pub trait ByteBufferExtRead {
    fn read_bool(&mut self) -> Result<bool>;
    // read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> Result<Vec<u8>>;

    fn read_value<T: Decodable>(&mut self) -> Result<T>;
    fn read_optional_value<T: Decodable>(&mut self) -> Result<Option<T>>;

    fn read_value_array<T: Decodable, const N: usize>(&mut self) -> Result<[T; N]>;
    fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>>;

    fn read_enum<E: TryFrom<B>, B: Decodable>(&mut self) -> Result<E>;

    fn read_color3(&mut self) -> Result<cocos::Color3B>;
    fn read_color4(&mut self) -> Result<cocos::Color4B>;
    fn read_point(&mut self) -> Result<cocos::Point>;
}

/* FastByteBuffer - zero heap allocation buffer for encoding but also limited functionality */
// will panic on writes if there isn't enough space.

pub struct FastByteBuffer<'a> {
    pos: usize,
    data: &'a mut [u8],
}

impl<'a> FastByteBuffer<'a> {
    // Create a new FastByteBuffer given this mutable slice
    pub fn new(src: &'a mut [u8]) -> Self {
        Self { pos: 0, data: src }
    }

    #[inline]
    pub fn write_u8(&mut self, val: u8) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_u16(&mut self, val: u16) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_u32(&mut self, val: u32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_u64(&mut self, val: u64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_i8(&mut self, val: i8) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_i16(&mut self, val: i16) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_i32(&mut self, val: i32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_i64(&mut self, val: i64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_f32(&mut self, val: f32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_f64(&mut self, val: f64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline]
    pub fn write_bytes(&mut self, data: &[u8]) {
        self.internal_write(data);
    }

    #[inline]
    pub fn write_string(&mut self, val: &str) {
        self.write_u32(val.len() as u32);
        self.write_bytes(val.as_bytes());
    }

    #[inline]
    pub fn as_bytes(&'a mut self) -> &'a [u8] {
        &self.data[..self.pos]
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.pos
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.pos == 0
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.data.len()
    }

    // Panics if there is not enough capacity left to write the data.
    fn internal_write(&mut self, data: &[u8]) {
        debug_assert!(
            self.pos + data.len() <= self.capacity(),
            "not enough space to write data into FastByteBuffer, capacity: {}, pos: {}, attempted write size: {}",
            self.capacity(),
            self.pos,
            data.len()
        );

        self.data[self.pos..self.pos + data.len()].copy_from_slice(data);
        self.pos += data.len();
    }
}

/* ByteBuffer extension implementation for ByteBuffer, ByteReader and FastByteBuffer */

impl ByteBufferExt for ByteBuffer {
    fn with_capacity(capacity: usize) -> Self {
        let mut ret = Self::from_vec(Vec::with_capacity(capacity));
        ret.set_wpos(0);
        ret
    }
}

macro_rules! impl_extwrite {
    ($encode_fn:ident) => {
        fn write_bool(&mut self, val: bool) {
            self.write_u8(u8::from(val));
        }

        fn write_byte_array(&mut self, val: &[u8]) {
            self.write_u32(val.len() as u32);
            self.write_bytes(val);
        }

        fn write_value<T: Encodable>(&mut self, val: &T) {
            val.$encode_fn(self);
        }

        fn write_optional_value<T: Encodable>(&mut self, val: Option<&T>) {
            self.write_bool(val.is_some());
            if let Some(val) = val {
                self.write_value(val);
            }
        }

        fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]) {
            val.iter().for_each(|v| self.write_value(v));
        }

        fn write_value_vec<T: Encodable>(&mut self, val: &[T]) {
            self.write_u32(val.len() as u32);
            for elem in val {
                elem.$encode_fn(self);
            }
        }

        fn write_enum<E: Into<B>, B: Encodable>(&mut self, val: E) {
            self.write_value(&val.into());
        }

        fn write_color3(&mut self, val: cocos::Color3B) {
            self.write_value(&val);
        }

        fn write_color4(&mut self, val: cocos::Color4B) {
            self.write_value(&val);
        }

        fn write_point(&mut self, val: cocos::Point) {
            self.write_value(&val);
        }
    };
}

macro_rules! impl_extread {
    ($decode_fn:ident) => {
        fn read_bool(&mut self) -> Result<bool> {
            Ok(self.read_u8()? != 0u8)
        }

        fn read_byte_array(&mut self) -> Result<Vec<u8>> {
            let length = self.read_u32()? as usize;
            Ok(self.read_bytes(length)?)
        }

        fn read_value<T: Decodable>(&mut self) -> Result<T> {
            T::$decode_fn(self)
        }

        fn read_optional_value<T: Decodable>(&mut self) -> Result<Option<T>> {
            Ok(match self.read_bool()? {
                false => None,
                true => Some(self.read_value::<T>()?),
            })
        }

        fn read_value_array<T: Decodable, const N: usize>(&mut self) -> Result<[T; N]> {
            array_init::try_array_init(|_| self.read_value::<T>())
        }

        fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>> {
            let mut out = Vec::new();
            let length = self.read_u32()? as usize;
            for _ in 0..length {
                out.push(self.read_value()?);
            }

            Ok(out)
        }

        fn read_enum<E: TryFrom<B>, B: Decodable>(&mut self) -> Result<E> {
            let val = self.read_value::<B>()?;
            let val: Result<E, _> = val.try_into();

            val.map_err(|_| anyhow!("failed to decode enum"))
        }

        fn read_color3(&mut self) -> Result<cocos::Color3B> {
            self.read_value()
        }

        fn read_color4(&mut self) -> Result<cocos::Color4B> {
            self.read_value()
        }

        fn read_point(&mut self) -> Result<cocos::Point> {
            self.read_value()
        }
    };
}

impl ByteBufferExtWrite for ByteBuffer {
    impl_extwrite!(encode);
}

impl<'a> ByteBufferExtWrite for FastByteBuffer<'a> {
    impl_extwrite!(encode_fast);
}

impl ByteBufferExtRead for ByteBuffer {
    impl_extread!(decode);
}

impl<'a> ByteBufferExtRead for ByteReader<'a> {
    impl_extread!(decode_from_reader);
}

/* Encodable/Decodable implementations for common types */

macro_rules! impl_primitive {
    ($typ:ty,$read:ident,$write:ident) => {
        impl Encodable for $typ {
            fn encode(&self, buf: &mut ByteBuffer) {
                buf.$write(*self);
            }

            fn encode_fast(&self, buf: &mut FastByteBuffer) {
                buf.$write(*self);
            }
        }

        impl Decodable for $typ {
            fn decode(buf: &mut ByteBuffer) -> Result<Self>
            where
                Self: Sized,
            {
                Ok(buf.$read()?)
            }
            fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
            where
                Self: Sized,
            {
                Ok(buf.$read()?)
            }
        }
    };
}

impl_primitive!(u8, read_u8, write_u8);
impl_primitive!(u16, read_u16, write_u16);
impl_primitive!(u32, read_u32, write_u32);
impl_primitive!(u64, read_u64, write_u64);
impl_primitive!(i8, read_i8, write_i8);
impl_primitive!(i16, read_i16, write_i16);
impl_primitive!(i32, read_i32, write_i32);
impl_primitive!(i64, read_i64, write_i64);

impl Encodable for Vec<u8> {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_byte_array(self);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_byte_array(self);
    }
}

impl Decodable for Vec<u8> {
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_byte_array()
    }

    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized,
    {
        buf.read_byte_array()
    }
}
