use std::fmt::Display;

use crate::data::{
    packets::{PacketHeader, PacketMetadata},
    types::cocos,
};
use bytebuffer::{ByteBuffer, ByteReader};

#[derive(Debug)]
pub enum DecodeError {
    NotEnoughData,
    NotEnoughCapacityString,
    InvalidEnumValue,
    InvalidStringValue,
}

impl Display for DecodeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NotEnoughData => f.write_str("could not read enough bytes from the ByteBuffer"),
            Self::NotEnoughCapacityString => f.write_str("not enough capacity to fit the given string into a FastString"),
            Self::InvalidEnumValue => f.write_str("invalid enum value was passed"),
            Self::InvalidStringValue => f.write_str("invalid string was passed, likely not properly UTF-8 encoded"),
        }
    }
}

impl From<std::io::Error> for DecodeError {
    fn from(_: std::io::Error) -> Self {
        DecodeError::NotEnoughData
    }
}

pub type DecodeResult<T> = core::result::Result<T, DecodeError>;

pub trait Encodable {
    fn encode(&self, buf: &mut ByteBuffer);
    fn encode_fast(&self, buf: &mut FastByteBuffer);
}

pub trait Decodable {
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized;
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized;
}

pub trait KnownSize {
    /// For dynamically sized types, this must be the maximum permitted size in the encoded form.
    /// If `FastByteBuffer::write` tries to write more bytes than this value, it may panic.
    const ENCODED_SIZE: usize;
}

/// maximum characters in a user's name (32). they can only be 15 chars max but we give headroom just in case
pub const MAX_NAME_SIZE: usize = 32;
/// maximum characters in a `ServerNoticePacket` or `ServerDisconnectPacket` (164)
pub const MAX_NOTICE_SIZE: usize = 164;
/// maximum characters in a user message (256)
pub const MAX_MESSAGE_SIZE: usize = 256;
/// max profiles that can be requested in `RequestProfilesPacket` (128)
pub const MAX_PROFILES_REQUESTED: usize = 128;

/// Simple and compact way of implementing `Decodable::decode` and `Decodable::decode_from_reader`.
///
/// Example usage:
/// ```rust
/// struct Type {
///     some_val: String,
/// }
///
/// decode_impl!(Type, buf, {
///     Ok(Self { some_val: buf.read()? })
/// });
/// ```
macro_rules! decode_impl {
    ($typ:ty, $buf:ident, $decode:expr) => {
        impl crate::data::Decodable for $typ {
            fn decode($buf: &mut bytebuffer::ByteBuffer) -> crate::data::DecodeResult<Self> {
                $decode
            }

            fn decode_from_reader($buf: &mut bytebuffer::ByteReader) -> crate::data::DecodeResult<Self> {
                $decode
            }
        }
    };
}

/// Simple and compact way of implementing `Encodable::encode` and `Encodable::encode_fast`.
///
/// Example usage:
/// ```rust
/// struct Type {
///     some_val: String,
/// }
///
/// encode_impl!(Type, buf, self, {
///     buf.write(&self.some_val);
/// });
/// ```
macro_rules! encode_impl {
    ($typ:ty, $buf:ident, $self:ident, $encode:expr) => {
        impl crate::data::Encodable for $typ {
            fn encode(&$self, $buf: &mut bytebuffer::ByteBuffer) {
                $encode
            }

            fn encode_fast(&$self, $buf: &mut crate::data::FastByteBuffer) {
                $encode
            }
        }
    };
}

/// Simple and compact way of implementing `KnownSize`.
///
/// Example usage:
/// ```rust
/// struct Type {
///     some_val: i32,
/// }
///
/// size_calc_impl!(Type, 4);
/// ```
macro_rules! size_calc_impl {
    ($typ:ty, $calc:expr) => {
        impl crate::data::bytebufferext::KnownSize for $typ {
            const ENCODED_SIZE: usize = $calc;
        }
    };
}

/// Simple way of getting total encoded size of given primitives.
///
/// Example usage:
/// ```rust
/// assert_eq!(16, size_of_primitives!(u64, i32, i16, i8, bool));
/// ```
macro_rules! size_of_primitives {
    ($($t:ty),+ $(,)?) => {{
        0 $(+ std::mem::size_of::<$t>())*
    }};
}

/// Simple way of getting total (maximum) encoded size of given types that implement `Encodable` and `KnownSize`
///
/// Example usage:
/// ```rust
/// let size = size_of_types!(u32, Option<PlayerData>, bool);
/// ```
macro_rules! size_of_types {
    ($($t:ty),+ $(,)?) => {{
        0 $(+ <$t>::ENCODED_SIZE)*
    }};
}

pub(crate) use decode_impl;
pub(crate) use encode_impl;
pub(crate) use size_calc_impl;
pub(crate) use size_of_primitives;
pub(crate) use size_of_types;

/* ByteBuffer extensions */

pub trait ByteBufferExt {
    fn with_capacity(capacity: usize) -> Self;
}

pub trait ByteBufferExtWrite {
    /// alias to `write_value`
    fn write<T: Encodable>(&mut self, val: &T) {
        self.write_value(val);
    }

    fn write_bool(&mut self, val: bool);
    /// write a `&[u8]`, prefixed with 4 bytes indicating length
    fn write_byte_array(&mut self, vec: &[u8]);

    fn write_value<T: Encodable>(&mut self, val: &T);
    fn write_optional_value<T: Encodable>(&mut self, val: Option<&T>);

    /// write an array `[T; N]`, without prefixing it with the total amount of values
    fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]);
    /// write a `Vec<T>`, prefixed with 4 bytes indicating the amount of values
    fn write_value_vec<T: Encodable>(&mut self, val: &[T]);

    fn write_packet_header<T: PacketMetadata>(&mut self);

    fn write_color3(&mut self, val: cocos::Color3B);
    fn write_color4(&mut self, val: cocos::Color4B);
    fn write_point(&mut self, val: cocos::Point);
}

pub trait ByteBufferExtRead {
    /// alias to `read_value`
    fn read<T: Decodable>(&mut self) -> DecodeResult<T> {
        self.read_value()
    }

    fn read_bool(&mut self) -> DecodeResult<bool>;
    /// read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> DecodeResult<Vec<u8>>;
    /// read the remaining data into a Vec
    fn read_remaining_bytes(&mut self) -> DecodeResult<Vec<u8>>;

    fn read_value<T: Decodable>(&mut self) -> DecodeResult<T>;
    fn read_optional_value<T: Decodable>(&mut self) -> DecodeResult<Option<T>>;

    /// read an array `[T; N]`, size must be known at compile time
    fn read_value_array<T: Decodable, const N: usize>(&mut self) -> DecodeResult<[T; N]>;
    /// read a `Vec<T>`
    fn read_value_vec<T: Decodable>(&mut self) -> DecodeResult<Vec<T>>;

    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader>;

    fn read_color3(&mut self) -> DecodeResult<cocos::Color3B>;
    fn read_color4(&mut self) -> DecodeResult<cocos::Color4B>;
    fn read_point(&mut self) -> DecodeResult<cocos::Point>;
}

/// Buffer for encoding that does zero heap allocation but also has limited functionality.
/// It will panic on writes if there isn't enough space.
pub struct FastByteBuffer<'a> {
    pos: usize,
    len: usize,
    data: &'a mut [u8],
}

impl<'a> FastByteBuffer<'a> {
    /// Create a new `FastByteBuffer` given this mutable slice
    pub fn new(src: &'a mut [u8]) -> Self {
        Self {
            pos: 0,
            len: 0,
            data: src,
        }
    }

    pub fn new_with_length(src: &'a mut [u8], len: usize) -> Self {
        Self { pos: 0, len, data: src }
    }

    pub fn write_u8(&mut self, val: u8) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_u16(&mut self, val: u16) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_u32(&mut self, val: u32) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_u64(&mut self, val: u64) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_i8(&mut self, val: i8) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_i16(&mut self, val: i16) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_i32(&mut self, val: i32) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_i64(&mut self, val: i64) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_f32(&mut self, val: f32) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_f64(&mut self, val: f64) {
        self.internal_write(&val.to_be_bytes());
    }

    pub fn write_bytes(&mut self, data: &[u8]) {
        self.internal_write(data);
    }

    pub fn write_string(&mut self, val: &str) {
        self.write_u32(val.len() as u32);
        self.write_bytes(val.as_bytes());
    }

    pub fn as_bytes(&'a mut self) -> &'a [u8] {
        &self.data[..self.len]
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn set_pos(&mut self, pos: usize) {
        self.pos = pos;
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn capacity(&self) -> usize {
        self.data.len()
    }

    /// Write the given byte slice. Panics if there is not enough capacity left to write the data.
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
        self.len = self.len.max(self.pos);
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

        fn write_packet_header<T: PacketMetadata>(&mut self) {
            self.write_value(&PacketHeader::from_packet::<T>());
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
        fn read_bool(&mut self) -> DecodeResult<bool> {
            Ok(self.read_u8()? != 0u8)
        }

        fn read_byte_array(&mut self) -> DecodeResult<Vec<u8>> {
            let length = self.read_u32()? as usize;
            Ok(self.read_bytes(length)?)
        }

        fn read_remaining_bytes(&mut self) -> DecodeResult<Vec<u8>> {
            let remainder = self.len() - self.get_rpos();
            let mut data = Vec::with_capacity(remainder);

            // safety: we don't allow to read any unitialized memory, as we trust the return value of `Read::read`
            unsafe {
                let ptr = data.as_mut_ptr();
                let mut slice = std::slice::from_raw_parts_mut(ptr, remainder);
                let len = std::io::Read::read(self, &mut slice)?;
                data.set_len(len);
            }

            Ok(data)
        }

        fn read_value<T: Decodable>(&mut self) -> DecodeResult<T> {
            T::$decode_fn(self)
        }

        fn read_optional_value<T: Decodable>(&mut self) -> DecodeResult<Option<T>> {
            Ok(match self.read_bool()? {
                false => None,
                true => Some(self.read_value::<T>()?),
            })
        }

        fn read_value_array<T: Decodable, const N: usize>(&mut self) -> DecodeResult<[T; N]> {
            array_init::try_array_init(|_| self.read_value::<T>())
        }

        fn read_value_vec<T: Decodable>(&mut self) -> DecodeResult<Vec<T>> {
            let mut out = Vec::new();
            let length = self.read_u32()? as usize;
            for _ in 0..length {
                out.push(self.read_value()?);
            }

            Ok(out)
        }

        fn read_packet_header(&mut self) -> DecodeResult<PacketHeader> {
            self.read_value()
        }

        fn read_color3(&mut self) -> DecodeResult<cocos::Color3B> {
            self.read_value()
        }

        fn read_color4(&mut self) -> DecodeResult<cocos::Color4B> {
            self.read_value()
        }

        fn read_point(&mut self) -> DecodeResult<cocos::Point> {
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
