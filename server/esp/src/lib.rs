//! esp - Binary serialization protocol library.
//! Provides traits `Encodable`, `Decodable` and `StaticSize` and implementations of those traits for many core types.
//!
//! Also re-exports `ByteBuffer` and `ByteReader` (extended w/ traits `ByteBufferReadExt` and `ByteBufferWriteExt`),
//! and its own `FastByteBuffer` which makes zero allocation on its own and can be used with stack/alloca arrays.
//!
//! esp also provides optimized types such as `InlineString` that will be more efficient in encoding/decoding,
//! and shall be used for encoding instead of the alternatives when possible.

#![feature(maybe_uninit_uninit_array, const_for, const_trait_impl, effects, const_mut_refs)]
#![allow(
    clippy::must_use_candidate,
    clippy::cast_possible_truncation,
    clippy::missing_errors_doc,
    clippy::missing_panics_doc,
    clippy::missing_safety_doc,
    clippy::wildcard_imports,
    clippy::module_name_repetitions
)]
use std::{fmt::Display, mem::MaybeUninit};
mod common;
mod fastbuffer;
pub mod hash;
pub mod types;

pub use bytebuffer::{ByteBuffer, ByteReader, Endian};
pub use fastbuffer::FastByteBuffer;
pub use types::*;

#[derive(Debug)]
pub enum DecodeError {
    NotEnoughData,
    NotEnoughCapacity,
    InvalidEnumValue,
    InvalidStringValue,
    NonFiniteValue,
    ChecksumMismatch,
}

impl Display for DecodeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NotEnoughData => f.write_str("could not read enough bytes from the ByteBuffer"),
            Self::NotEnoughCapacity => f.write_str("not enough capacity to fit the given value into a InlineString or FastVec"),
            Self::InvalidEnumValue => f.write_str("invalid enum value was passed"),
            Self::InvalidStringValue => f.write_str("invalid string was passed, likely not properly UTF-8 encoded"),
            Self::NonFiniteValue => f.write_str("NaN or inf was passed as a data field expecting a finite f32 or f64 value"),
            Self::ChecksumMismatch => f.write_str("data checksum was invalid"),
        }
    }
}

impl From<std::io::Error> for DecodeError {
    fn from(_: std::io::Error) -> Self {
        Self::NotEnoughData
    }
}

pub type DecodeResult<T> = core::result::Result<T, DecodeError>;
pub type VarLength = u16;

pub trait Encodable {
    fn encode(&self, buf: &mut ByteBuffer);
    fn encode_fast(&self, buf: &mut FastByteBuffer);
}

pub trait Decodable {
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let data = &buf.as_bytes()[buf.get_rpos()..];
        let mut reader = ByteReader::from_bytes(data);

        let val = Self::decode_from_reader(&mut reader);

        // transfer rpos
        buf.set_rpos(buf.get_rpos() + reader.get_rpos());

        val
    }

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
/// use esp::*;
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
/// use esp::*;
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
/// use esp::*;
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
    };
}

#[macro_export]
macro_rules! dynamic_size_calc_impl {
    ($typ:ty, $self:ident, $calc:expr) => {
        impl $crate::DynamicSize for $typ {
            #[inline]
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
/// use esp::*;
///
/// let size = size_of_types!(u32, Option<i32>, bool);
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

#[macro_export]
macro_rules! dynamic_size_as_static_impl {
    ($typ:ty) => {
        dynamic_size_calc_impl!($typ, self, Self::ENCODED_SIZE);
    };
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

    /// write a `u16`, panicking if `val` does not fit into 2 bytes.
    fn write_length(&mut self, val: usize);

    fn write_bool(&mut self, val: bool);
    /// write a `&[u8]`, prefixed with 4 bytes indicating length
    fn write_byte_array(&mut self, vec: &[u8]);

    fn write_value<T: Encodable + ?Sized>(&mut self, val: &T);

    /// write an array `[T; N]`, without prefixing it with the total amount of values
    fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]);

    /// calculate the checksum of the entire buffer and write it at the current position (4 bytes)
    fn append_self_checksum(&mut self);
}

pub trait ByteBufferExtRead {
    /// alias to `read_value`
    fn read<T: Decodable>(&mut self) -> DecodeResult<T> {
        self.read_value()
    }

    /// skip the next `n` bytes
    fn skip(&mut self, n: usize);

    fn read_bool(&mut self) -> DecodeResult<bool>;

    /// read a length of a datatype (encoded as `u16`)
    fn read_length(&mut self) -> DecodeResult<usize>;

    /// read a number `x` of type `u16`, and return an error if there's less than `x * sizeof(T)` bytes available in the buffer
    fn read_length_check<T: StaticSize>(&mut self) -> DecodeResult<usize>;

    /// read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> DecodeResult<Vec<u8>>;
    /// read the remaining data into a Vec
    fn read_remaining_bytes(&mut self) -> DecodeResult<Vec<u8>>;

    fn read_value<T: Decodable>(&mut self) -> DecodeResult<T>;

    /// read an array `[T; N]`, size must be known at compile time
    fn read_value_array<T: Decodable, const N: usize>(&mut self) -> DecodeResult<[T; N]>;

    /// read the checksum at the end of the buffer (4 bytes) and verify the rest of the buffer matches
    fn validate_self_checksum(&mut self) -> DecodeResult<()>;
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

        fn write_length(&mut self, val: usize) {
            debug_assert!(
                val < u16::MAX as usize,
                "attempting to call write_length with a value not fitting into 2 bytes ({val})"
            );

            self.write_u16(val as u16);
        }

        #[inline]
        fn write_byte_array(&mut self, val: &[u8]) {
            self.write_length(val.len());
            self.write_bytes(val);
        }

        #[inline]
        fn write_value<T: Encodable + ?Sized>(&mut self, val: &T) {
            val.$encode_fn(self);
        }

        #[inline]
        fn write_value_array<T: Encodable, const N: usize>(&mut self, val: &[T; N]) {
            for elem in val {
                self.write_value(elem);
            }
        }

        #[inline]
        fn append_self_checksum(&mut self) {
            // TODO: enable adler
            // let mut adler = adler32fast::Adler32::new();
            // adler.update(self.as_bytes());
            // let checksum = adler.as_u32();
            let checksum = crc32fast::hash(self.as_bytes());

            self.write_u32(checksum);
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
        fn read_length(&mut self) -> DecodeResult<usize> {
            Ok(self.read_u16()? as usize)
        }

        #[inline]
        fn read_length_check<T: StaticSize>(&mut self) -> DecodeResult<usize> {
            let len = self.read_length()?;

            if self.get_rpos() + (len * T::ENCODED_SIZE) > self.len() {
                Err(DecodeError::NotEnoughData)
            } else {
                Ok(len)
            }
        }

        #[inline]
        fn read_byte_array(&mut self) -> DecodeResult<Vec<u8>> {
            let length = self.read_length()? as usize;
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
        fn read_value_array<T: Decodable, const N: usize>(&mut self) -> DecodeResult<[T; N]> {
            // [(); N].try_map(|_| self.read_value::<T>())
            // ^^ i would love to only use safe rust but a ~10% performance difference is a bit too big to ignore

            let mut arr = MaybeUninit::<T>::uninit_array::<N>();
            for i in 0..N {
                match self.read_value::<T>() {
                    Ok(val) => arr[i].write(val),
                    Err(err) => {
                        // cleanup the previous values and return the error
                        for j in 0..i {
                            // safety: we know that `j < i`, and we know that all elements in `arr` that are before `arr[i]` must already be initialized
                            unsafe {
                                arr[j].assume_init_drop();
                            }
                        }

                        return Err(err);
                    }
                };
            }

            // safety: we have initialized all values as seen above. if decoding failed at any moment, this step wouldn't be reachable.
            Ok(arr.map(|x| unsafe { x.assume_init() }))
        }

        #[inline]
        fn validate_self_checksum(&mut self) -> DecodeResult<()> {
            if self.len() < 4 {
                return Err(DecodeError::ChecksumMismatch);
            }

            let last_pos = self.get_rpos();
            let before_cksum = self.len() - 4;

            self.set_rpos(before_cksum);
            let checksum = self.read_u32()?;

            // TODO: enable adler
            // let mut adler = adler32fast::Adler32::new();
            // adler.update(&self.as_bytes()[..before_cksum]);
            // let correct_checksum = adler.as_u32();
            let correct_checksum = crc32fast::hash(&self.as_bytes()[..before_cksum]);

            self.set_rpos(last_pos);

            if checksum != correct_checksum {
                return Err(DecodeError::ChecksumMismatch);
            }

            Ok(())
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

// Tests.

#[cfg(test)]
mod tests {
    use crate as esp;
    use crate::*;
    use globed_derive::*;

    #[test]
    fn inline_string() {
        let mut buf = ByteBuffer::new();
        let string = InlineString::<120>::new("hello there, this is a basic test string!");

        buf.write_value(&string);
        buf.set_rpos(0);
        let out = buf.read_value::<InlineString<120>>();

        assert!(out.is_ok(), "failed to read the string");
        assert_eq!(out.unwrap(), string);
    }

    #[test]
    fn checksum() {
        let mut buf = ByteBuffer::new();
        buf.write_u32(0x9d);
        buf.write_u32(0x9a);
        buf.write_u32(0x94);
        buf.write_u32(0x91);
        buf.append_self_checksum();

        let mut reader = ByteReader::from_bytes(buf.as_bytes());
        assert!(reader.validate_self_checksum().is_ok(), "failed to validate checksum");
    }

    #[test]
    fn fast_vec() {
        let mut vec = FastVec::<String, 128>::new();
        vec.push("test string 1".to_owned());
        vec.push("test string 2".to_owned());
        vec.push("test string 3".to_owned());
        vec.push("test string 4".to_owned());
        vec.push("test string 5".to_owned());

        let mut buffer = ByteBuffer::new();
        buffer.write_value(&vec);

        buffer.set_rpos(0);
        let value = buffer.read_value::<FastVec<String, 128>>();

        assert!(value.is_ok(), "failed to read FastVec");

        let vec = value.unwrap();

        let mut viter = vec.iter();
        assert_eq!(viter.next().unwrap(), &"test string 1".to_owned());
        assert_eq!(viter.next().unwrap(), &"test string 2".to_owned());
        assert_eq!(viter.next().unwrap(), &"test string 3".to_owned());
        assert_eq!(viter.next().unwrap(), &"test string 4".to_owned());
        assert_eq!(viter.next().unwrap(), &"test string 5".to_owned());

        assert!(viter.next().is_none(), "vector didn't end");
    }

    #[derive(Encodable, Decodable, StaticSize, DynamicSize)]
    #[bitfield(on = true)]
    #[allow(clippy::struct_excessive_bools)]
    struct FlagStruct {
        b0: bool,
        b1: bool,
        b2: bool,
        b3: bool,
        b4: bool,
        b5: bool,
        b6: bool,
        b7: bool,
        b8: bool,
    }

    #[derive(Encodable, Decodable, StaticSize, DynamicSize)]
    #[bitfield(on = true, size = 8)]
    #[allow(clippy::struct_excessive_bools)]
    struct FlagStruct2 {
        b1: bool,
        b2: bool,
        b3: bool,
        b4: bool,
    }

    #[test]
    fn bitfield() {
        assert_eq!(FlagStruct::ENCODED_SIZE, 2);
        assert_eq!(FlagStruct2::ENCODED_SIZE, 8);

        let mut bits = Bits::<9>::new();
        bits.set_bit(1);
        bits.set_bit(3);
        bits.set_bit(5);
        bits.set_bit(7);

        let mut buf = ByteBuffer::new();
        buf.write_value(&bits);

        let flags = buf.read_value::<FlagStruct>().unwrap();

        assert!(flags.b1 && flags.b3 && flags.b5 && flags.b7);
        assert!(!flags.b0 && !flags.b2 && !flags.b4 && !flags.b6);

        buf.clear();
        buf.set_wpos(0);
        buf.write_value(&flags);

        buf.set_rpos(0);

        let flags2 = buf.read_value::<FlagStruct2>();

        assert_eq!(buf.len(), FlagStruct::ENCODED_SIZE);

        assert!(flags2.is_err());
    }

    #[test]
    #[allow(clippy::char_lit_as_u8)]
    fn fast_string() {
        let mut s = FastString::new("test data");
        assert!(!s.is_heap());
        assert_eq!(s.try_to_str(), "test data");
        assert_eq!(s.len(), 9);

        s.push(' ' as u8);
        s.extend("even more test data");
        assert_eq!(s.len(), 29);

        assert_eq!(s.try_to_str(), "test data even more test data");

        assert!(!s.is_heap());

        s.extend("even even even even even even even even even even even even even even even more test data");
        assert!(s.is_heap());

        assert_eq!(
            s.try_to_str(),
            "test data even more test dataeven even even even even even even even even even even even even even even more test data"
        );
    }
}
