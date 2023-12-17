//! esp - Binary serialization protocol library.
//! Provides traits `Encodable`, `Decodable` and `StaticSize` and implementations of those traits for many core types.
//!
//! Also re-exports `ByteBuffer` and `ByteReader` (extended w/ traits `ByteBufferReadExt` and `ByteBufferWriteExt`),
//! and its own `FastByteBuffer` which makes zero allocation on its own and can be used with stack/alloca arrays.
//!
//! esp also provides optimized types such as `FastString` that will be more efficient in encoding/decoding,
//! and shall be used for encoding instead of the alternatives when possible.

#![feature(maybe_uninit_uninit_array)]
#![allow(
    clippy::must_use_candidate,
    clippy::cast_possible_truncation,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::missing_safety_doc,
    clippy::wildcard_imports
)]
use std::{fmt::Display, mem::MaybeUninit};
mod common;
mod fastbuffer;
pub mod types;

pub use bytebuffer::{ByteBuffer, ByteReader, Endian};
pub use common::*;
pub use fastbuffer::FastByteBuffer;

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

pub trait StaticSize {
    /// For dynamically sized types, this is the maximum permitted size in the encoded form.
    /// If `FastByteBuffer::write` tries to write more bytes than this value, it may panic.
    const ENCODED_SIZE: usize;
}

pub trait DynamicSize {
    /// For types that have an unpredicatble size at compile time (`Vec`, `String`, `HashMap`, etc.),
    /// this method can be used to calculate the maximum permitted encoded size at runtime.
    fn encoded_size(&self) -> usize;
}

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
#[macro_export]
macro_rules! decode_impl {
    ($typ:ty, $buf:ident, $decode:expr) => {
        impl $crate::Decodable for $typ {
            #[inline]
            fn decode($buf: &mut $crate::ByteBuffer) -> $crate::DecodeResult<Self> {
                $decode
            }
            #[inline]
            fn decode_from_reader($buf: &mut $crate::ByteReader) -> $crate::DecodeResult<Self> {
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
#[macro_export]
macro_rules! encode_impl {
    ($typ:ty, $buf:ident, $self:ident, $encode:expr) => {
        impl $crate::Encodable for $typ {
            #[inline]
            fn encode(&$self, $buf: &mut $crate::ByteBuffer) {
                $encode
            }
            #[inline]
            fn encode_fast(&$self, $buf: &mut $crate::FastByteBuffer) {
                $encode
            }
        }
    };
}

/// Simple and compact way of implementing `StaticSize`.
///
/// Example usage:
/// ```rust
/// struct Type {
///     some_val: i32,
/// }
///
/// static_size_calc_impl!(Type, 4);
/// ```
#[macro_export]
macro_rules! static_size_calc_impl {
    ($typ:ty, $calc:expr) => {
        impl $crate::StaticSize for $typ {
            const ENCODED_SIZE: usize = $calc;
        }

        impl $crate::DynamicSize for $typ {
            #[inline(always)]
            fn encoded_size(&self) -> usize {
                Self::ENCODED_SIZE
            }
        }
    };
}

#[macro_export]
macro_rules! dynamic_size_calc_impl {
    ($typ:ty, $self:ident, $calc:expr) => {
        impl $crate::DynamicSize for $typ {
            #[inline(always)]
            fn encoded_size(&$self) -> usize {
                $calc
            }
        }
    };
}

/// Simple way of getting total (maximum) encoded size of given types that implement `Encodable` and `StaticSize`
///
/// Example usage:
/// ```rust
/// let size = size_of_types!(u32, Option<PlayerData>, bool);
/// ```
#[macro_export]
macro_rules! size_of_types {
    ($($t:ty),+ $(,)?) => {{
        0 $(+ <$t>::ENCODED_SIZE)*
    }};
}

#[macro_export]
macro_rules! size_of_dynamic_types {
    ($($t:expr),+ $(,)?) => {{
        0 $(+ DynamicSize::encoded_size($t))*
    }};
}

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
}

pub trait ByteBufferExtRead {
    /// alias to `read_value`
    fn read<T: Decodable>(&mut self) -> DecodeResult<T> {
        self.read_value()
    }

    /// skip the next `n` bytes
    fn skip(&mut self, n: usize);

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
}

/* ByteBuffer extension implementation for ByteBuffer, ByteReader and FastByteBuffer */

impl ByteBufferExt for ByteBuffer {
    fn with_capacity(capacity: usize) -> Self {
        Self::from_vec(Vec::with_capacity(capacity))
    }
}

macro_rules! impl_extwrite {
    ($encode_fn:ident) => {
        #[inline]
        fn write_bool(&mut self, val: bool) {
            self.write_u8(u8::from(val));
        }

        #[inline]
        fn write_byte_array(&mut self, val: &[u8]) {
            self.write_u32(val.len() as u32);
            self.write_bytes(val);
        }

        #[inline]
        fn write_value<T: Encodable>(&mut self, val: &T) {
            val.$encode_fn(self);
        }

        #[inline]
        fn write_optional_value<T: Encodable>(&mut self, val: Option<&T>) {
            self.write_bool(val.is_some());
            if let Some(val) = val {
                self.write_value(val);
            }
        }

        #[inline]
        fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]) {
            for elem in val {
                self.write_value(elem);
            }
        }

        #[inline]
        fn write_value_vec<T: Encodable>(&mut self, val: &[T]) {
            self.write_u32(val.len() as u32);
            for elem in val {
                self.write_value(elem);
            }
        }
    };
}

macro_rules! impl_extread {
    ($decode_fn:ident) => {
        #[inline]
        fn skip(&mut self, n: usize) {
            self.set_rpos(self.get_rpos() + n);
        }

        #[inline]
        fn read_bool(&mut self) -> DecodeResult<bool> {
            Ok(self.read_u8()? != 0u8)
        }

        #[inline]
        fn read_byte_array(&mut self) -> DecodeResult<Vec<u8>> {
            let length = self.read_u32()? as usize;
            Ok(self.read_bytes(length)?)
        }

        #[inline]
        fn read_remaining_bytes(&mut self) -> DecodeResult<Vec<u8>> {
            let remainder = self.len() - self.get_rpos();
            let mut data = Vec::with_capacity(remainder);

            // safety: we use `Vec::set_len` appropriately so the caller won't be able to read uninitialized data.
            // we could avoid unsafe and use io::Read::read_to_end here, but that is significantly slower.
            unsafe {
                let ptr = data.as_mut_ptr();
                let mut slice = std::slice::from_raw_parts_mut(ptr, remainder);
                let len = std::io::Read::read(self, &mut slice)?;
                data.set_len(len);
            }

            Ok(data)
        }

        #[inline]
        fn read_value<T: Decodable>(&mut self) -> DecodeResult<T> {
            T::$decode_fn(self)
        }

        #[inline]
        fn read_optional_value<T: Decodable>(&mut self) -> DecodeResult<Option<T>> {
            Ok(match self.read_bool()? {
                false => None,
                true => Some(self.read_value::<T>()?),
            })
        }

        #[inline]
        fn read_value_array<T: Decodable, const N: usize>(&mut self) -> DecodeResult<[T; N]> {
            // [(); N].try_map(|_| self.read_value::<T>())
            // ^^ i would love to only use safe rust but a ~10% performance difference is a bit too big to ignore

            let mut a = MaybeUninit::<T>::uninit_array::<N>();
            for i in 0..N {
                a[i].write(self.read_value::<T>()?);
            }

            // safety: we have initialized all values as seen above. if decoding failed at any moment, this step woudln't be reachable.
            Ok(a.map(|x| unsafe { x.assume_init() }))
        }

        #[inline]
        fn read_value_vec<T: Decodable>(&mut self) -> DecodeResult<Vec<T>> {
            let mut out = Vec::new();
            let length = self.read_u32()? as usize;
            for _ in 0..length {
                out.push(self.read_value()?);
            }

            Ok(out)
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
