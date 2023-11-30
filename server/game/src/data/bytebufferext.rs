use crate::data::{
    packets::{PacketHeader, PacketMetadata},
    types::cocos,
};
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};

pub trait Encodable {
    /// write `Self` into the given buffer
    fn encode(&self, buf: &mut ByteBuffer);
    /// write `Self` into the given buffer, except blazingly fast this time
    fn encode_fast(&self, buf: &mut FastByteBuffer);
}

pub trait Decodable {
    /// read `Self` from the given `ByteBuffer`
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized;
    /// read `Self` from the given `ByteReader`
    fn decode_from_reader(buf: &mut ByteReader) -> Result<Self>
    where
        Self: Sized;
}

pub trait EncodableWithKnownSize: Encodable {
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
        impl crate::data::bytebufferext::Decodable for $typ {
            fn decode($buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self> {
                $decode
            }

            fn decode_from_reader($buf: &mut bytebuffer::ByteReader) -> anyhow::Result<Self> {
                $decode
            }
        }
    };
}

/// Implements `Decodable::decode` and `Decodable::decode_from_reader` to panic when called
macro_rules! decode_unimpl {
    ($typ:ty) => {
        impl crate::data::bytebufferext::Decodable for $typ {
            fn decode(_: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self> {
                panic!(
                    "Tried to call {}::decode when Decodable was not implemented for this type",
                    stringify!($typ)
                );
            }

            fn decode_from_reader(_: &mut bytebuffer::ByteReader) -> anyhow::Result<Self> {
                panic!(
                    "Tried to call {}::decode_from_reader when Decodable was not implemented for this type",
                    stringify!($typ)
                );
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
        impl crate::data::bytebufferext::Encodable for $typ {
            fn encode(&$self, $buf: &mut bytebuffer::ByteBuffer) {
                $encode
            }

            fn encode_fast(&$self, $buf: &mut crate::data::bytebufferext::FastByteBuffer) {
                $encode
            }
        }
    };
}

/// Implements `Encodable::encode` and `Encodable::encode_fast` to panic when called
macro_rules! encode_unimpl {
    ($typ:ty) => {
        impl crate::data::bytebufferext::Encodable for $typ {
            fn encode(&self, _: &mut bytebuffer::ByteBuffer) {
                panic!(
                    "Tried to call {}::encode when Encodable was not implemented for this type",
                    stringify!($typ)
                );
            }

            fn encode_fast(&self, _: &mut crate::data::bytebufferext::FastByteBuffer) {
                panic!(
                    "Tried to call {}::encode_fast when Encodable was not implemented for this type",
                    stringify!($typ)
                );
            }
        }
    };
}

/// Simple and compact way of implementing `EncodableWithKnownSize`.
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
        impl crate::data::bytebufferext::EncodableWithKnownSize for $typ {
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

/// Simple way of getting total (maximum) encoded size of given types that implement `Encodable` and `EncodableWithKnownSize`
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
pub(crate) use decode_unimpl;
pub(crate) use encode_impl;
pub(crate) use encode_unimpl;
pub(crate) use size_calc_impl;
pub(crate) use size_of_primitives;
pub(crate) use size_of_types;

/* ByteBuffer extensions
*
* With great power comes great responsibility.
* Just because you can use .write(T) and .read<T>() for every single type,
* even primitives (as they impl Encodable/Decodable), doesn't mean you should.
*
* Notable exception is Option, `write` will accept &Option<T> while `write_optional_value` will accept Option<&T>
* so feel free to use whichever method suits you more when dealing with those.
*/

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

    fn write_enum<E: Into<B>, B: Encodable>(&mut self, val: E);
    fn write_packet_header<T: PacketMetadata>(&mut self);

    fn write_color3(&mut self, val: cocos::Color3B);
    fn write_color4(&mut self, val: cocos::Color4B);
    fn write_point(&mut self, val: cocos::Point);
}

pub trait ByteBufferExtRead {
    /// alias to `read_value`
    fn read<T: Decodable>(&mut self) -> Result<T> {
        self.read_value()
    }

    fn read_bool(&mut self) -> Result<bool>;
    /// read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> Result<Vec<u8>>;
    /// read the remaining data into a Vec
    fn read_remaining_bytes(&mut self) -> Result<Vec<u8>>;

    fn read_value<T: Decodable>(&mut self) -> Result<T>;
    fn read_optional_value<T: Decodable>(&mut self) -> Result<Option<T>>;

    /// read an array `[T; N]`, size must be known at compile time
    fn read_value_array<T: Decodable, const N: usize>(&mut self) -> Result<[T; N]>;
    /// read a `Vec<T>`
    fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>>;

    fn read_enum<E: TryFrom<B>, B: Decodable>(&mut self) -> Result<E>;
    fn read_packet_header(&mut self) -> Result<PacketHeader>;

    fn read_color3(&mut self) -> Result<cocos::Color3B>;
    fn read_color4(&mut self) -> Result<cocos::Color4B>;
    fn read_point(&mut self) -> Result<cocos::Point>;
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

        fn write_enum<E: Into<B>, B: Encodable>(&mut self, val: E) {
            self.write_value(&val.into());
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
        fn read_bool(&mut self) -> Result<bool> {
            Ok(self.read_u8()? != 0u8)
        }

        fn read_byte_array(&mut self) -> Result<Vec<u8>> {
            let length = self.read_u32()? as usize;
            Ok(self.read_bytes(length)?)
        }

        fn read_remaining_bytes(&mut self) -> Result<Vec<u8>> {
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

        fn read_packet_header(&mut self) -> Result<PacketHeader> {
            self.read_value()
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

encode_impl!(Vec<u8>, buf, self, buf.write_byte_array(self));
decode_impl!(Vec<u8>, buf, buf.read_byte_array());

encode_impl!(String, buf, self, buf.write_string(self));
decode_impl!(String, buf, Ok(buf.read_string()?));

encode_impl!(&str, buf, self, buf.write_string(self));

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
