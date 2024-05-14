use std::{fmt, ops::Deref, str::Utf8Error};

use crate::*;

/// `InlineString` is a string class that doesn't use heap allocation like `String` but also owns the string (unlike `&str`).
/// When encoding or decoding into a byte buffer of any kind, the encoded form is identical to a normal `String`,
/// and they can be converted between each other interchangably with `.try_into()` or `.try_from()`.
///
/// Note: `InlineString` is not guaranteed to hold a valid UTF-8 string at any point in time, and there are no UTF-8 checks when decoding.
/// Checks are only done when you convert the string into a `&str` or `String`, via `.to_str`, `.try_to_str`, or `to_string`.
#[derive(Clone, Debug)]
pub struct InlineString<const N: usize> {
    buffer: [u8; N],
    len: usize,
}

impl<const N: usize> InlineString<N> {
    #[inline]
    pub fn new(data: &str) -> Self {
        Self::from_slice(data.as_bytes())
    }

    #[inline]
    pub const fn from_buffer(buffer: [u8; N], len: usize) -> Self {
        Self { buffer, len }
    }

    #[inline]
    pub fn from_slice(data: &[u8]) -> Self {
        debug_assert!(
            data.len() <= N,
            "Attempting to create a InlineString with {} bytes which is more than the capacity ({N})",
            data.len()
        );
        let mut buffer = [0u8; N];
        buffer[..data.len()].copy_from_slice(data);
        Self { buffer, len: data.len() }
    }

    #[inline]
    pub const fn len(&self) -> usize {
        self.len
    }

    #[inline]
    pub const fn is_empty(&self) -> bool {
        self.len == 0
    }

    #[inline]
    pub const fn capacity() -> usize {
        N
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        if self.len < N {
            self.buffer[self.len] = c;
            self.len += 1;
        } else {
            panic!("InlineString buffer overflow (writing beyond capacity of {N})");
        }
    }

    #[inline]
    pub fn extend(&mut self, data: &str) {
        debug_assert!(
            self.len + data.len() < Self::capacity(),
            "InlineString buffer overflow (current size is {}/{}, cannot extend with a string of length {})",
            self.len,
            Self::capacity(),
            data.len()
        );

        self.buffer[self.len..self.len + data.len()].copy_from_slice(data.as_bytes());
        self.len += data.len();
    }

    /// like `extend` but will simply truncate the data instead of panicking if the string doesn't fit
    #[inline]
    pub fn extend_safe(&mut self, data: &str) {
        for char in data.as_bytes() {
            if self.len >= N {
                break;
            }

            self.push(*char);
        }
    }

    #[inline]
    pub fn to_string(&self) -> Result<String, Utf8Error> {
        self.to_str().map(str::to_owned)
    }

    #[inline]
    pub fn to_str(&self) -> Result<&str, Utf8Error> {
        std::str::from_utf8(&self.buffer[..self.len])
    }

    /// Attempts to convert this string to a string slice, on failure returns "\<invalid UTF-8 string\>"
    #[inline]
    pub fn try_to_str(&self) -> &str {
        self.to_str().unwrap_or("<invalid UTF-8 string>")
    }

    #[inline]
    pub fn try_to_string(&self) -> String {
        self.to_str()
            .map_or_else(|_| "<invalid UTF-8 string>".to_owned(), ToOwned::to_owned)
    }

    /// Converts this string to a string slice, without doing any UTF-8 checks.
    /// If the string is not a valid UTF-8 string, the behavior is undefined.
    #[inline]
    pub unsafe fn to_str_unchecked(&self) -> &str {
        // in debug mode we still do a sanity check, a panic will indicate a *massive* logic error somewhere in the code.
        #[cfg(debug_assertions)]
        let ret = self
            .to_str()
            .expect("Attempted to unsafely convert a non-UTF-8 InlineString into a string slice");

        #[cfg(not(debug_assertions))]
        let ret = std::str::from_utf8_unchecked(&self.buffer[..self.len]);

        ret
    }

    /// Compares this string to `other` in constant time
    pub fn constant_time_compare(&self, other: &InlineString<N>) -> bool {
        if self.len != other.len {
            return false;
        }

        let mut result = 0u8;

        self.buffer
            .iter()
            .take(self.len)
            .zip(other.buffer.iter().take(other.len))
            .for_each(|(c1, c2)| result |= c1 ^ c2);

        result == 0
    }
}

impl<const N: usize> Encodable for InlineString<N> {
    #[inline]
    fn encode(&self, buf: &mut crate::ByteBuffer) {
        buf.write_u32(self.len as u32);
        buf.write_bytes(&self.buffer[..self.len]);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_u32(self.len as u32);
        buf.write_bytes(&self.buffer[..self.len]);
    }
}

impl<const N: usize> Decodable for InlineString<N> {
    #[inline]
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let len = buf.read_u32()? as usize;
        if len > N {
            return Err(DecodeError::NotEnoughCapacity);
        }

        let mut buffer = [0u8; N];
        std::io::Read::read(buf, &mut buffer[..len])?;

        Ok(Self::from_buffer(buffer, len))
    }

    #[inline]
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let len = buf.read_u32()? as usize;
        if len > N {
            return Err(DecodeError::NotEnoughCapacity);
        }

        let mut buffer = [0u8; N];
        std::io::Read::read(buf, &mut buffer[..len])?;

        Ok(Self::from_buffer(buffer, len))
    }
}

impl<const N: usize> StaticSize for InlineString<N> {
    const ENCODED_SIZE: usize = size_of_types!(u32) + N;
}

impl<const N: usize> DynamicSize for InlineString<N> {
    #[inline]
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.len
    }
}

impl<const N: usize> Deref for InlineString<N> {
    type Target = str;
    fn deref(&self) -> &Self::Target {
        self.try_to_str()
    }
}

impl<const N: usize> fmt::Display for InlineString<N> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self)
    }
}

impl<const N: usize> TryInto<String> for InlineString<N> {
    type Error = DecodeError;
    fn try_into(self) -> DecodeResult<String> {
        self.to_string().map_err(|_| DecodeError::InvalidStringValue)
    }
}

impl<const N: usize> TryFrom<String> for InlineString<N> {
    type Error = DecodeError;
    fn try_from(value: String) -> DecodeResult<Self> {
        TryFrom::<&str>::try_from(&value)
    }
}

impl<const N: usize> TryFrom<&str> for InlineString<N> {
    type Error = DecodeError;
    fn try_from(value: &str) -> DecodeResult<Self> {
        if value.len() > N {
            return Err(DecodeError::NotEnoughCapacity);
        }

        Ok(Self::new(value))
    }
}

impl<const N: usize> Default for InlineString<N> {
    fn default() -> Self {
        Self::from_buffer([0; N], 0)
    }
}

impl<const N: usize> PartialEq for InlineString<N> {
    fn eq(&self, other: &Self) -> bool {
        if other.len != self.len {
            return false;
        }

        self.buffer.iter().take(self.len).eq(other.buffer.iter().take(other.len))
    }
}

impl<const N: usize> Eq for InlineString<N> {}
