use std::ops::Deref;
use std::str::Utf8Error;

use crate::*;

/// `FastString` is an SSO string class, that can store up to `FastString::inline_capacity()` bytes inline,
/// and otherwise requires a heap allocation.
///
/// Just like `InlineString`, it can be converted to and from a `String` or `&str`.
///
pub struct FastString {
    repr: StringRepr,
}

const INLINE_CAP: usize = 62;

#[derive(Clone, Debug)]
enum StringRepr {
    Inline(InlineRepr),
    Heap(HeapRepr),
}

#[derive(Copy, Clone, Debug)]
struct InlineRepr {
    buf: [u8; INLINE_CAP],
    length: u8,
}

#[derive(Clone, Debug)]
struct HeapRepr {
    data: Vec<u8>,
}

impl InlineRepr {
    #[inline]
    pub fn len(&self) -> usize {
        self.length as usize
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        &self.buf[..self.len()]
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        debug_assert!(self.len() < INLINE_CAP, "FastString overflow");

        let idx = self.len();
        self.buf[idx] = c;
        self.length += 1;
    }

    #[inline]
    pub fn extend_bytes(&mut self, data: &[u8]) {
        let idx = self.len();

        debug_assert!(idx + data.len() < INLINE_CAP, "FastString overflow");

        self.buf[idx..idx + data.len()].copy_from_slice(&data[..data.len()]);
        self.length += data.len() as u8;
    }

    #[inline]
    pub fn clear(&mut self) {
        self.length = 0;
    }
}

impl HeapRepr {
    #[inline]
    fn new(data: &[u8]) -> Self {
        let len = data.len();
        // thats a fucking thing?? i love you rust
        let cap = len.next_power_of_two();

        let mut buf = Vec::with_capacity(cap);
        buf.extend_from_slice(data);

        Self { data: buf }
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.data.len()
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.data.capacity()
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        &self.data
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        self.data.push(c);
    }

    #[inline]
    pub fn extend_bytes(&mut self, data: &[u8]) {
        self.data.extend(data);
    }

    #[inline]
    pub fn clear(&mut self) {
        self.data.clear();
    }
}

impl StringRepr {
    #[inline]
    fn is_inline(&self) -> bool {
        match self {
            Self::Inline(_) => true,
            Self::Heap(_) => false,
        }
    }

    #[inline]
    fn new_with_heap(data: &[u8]) -> Self {
        Self::Heap(HeapRepr::new(data))
    }

    #[inline]
    fn new_with_inline(data: &[u8]) -> Self {
        let len = data.len();

        debug_assert!(len < INLINE_CAP);

        let mut buf = [0u8; INLINE_CAP];

        buf[..len].copy_from_slice(&data[..len]);

        Self::Inline(InlineRepr { buf, length: len as u8 })
    }

    pub const fn inline_capacity() -> usize {
        INLINE_CAP
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        match self {
            Self::Inline(_) => Self::inline_capacity(),
            Self::Heap(x) => x.capacity(),
        }
    }

    #[inline]
    pub fn len(&self) -> usize {
        match self {
            Self::Inline(x) => x.len(),
            Self::Heap(x) => x.len(),
        }
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        match self {
            Self::Inline(x) => x.is_empty(),
            Self::Heap(x) => x.len() == 0,
        }
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        match self {
            Self::Inline(x) => x.as_bytes(),
            Self::Heap(x) => x.as_bytes(),
        }
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        match self {
            Self::Inline(x) => {
                // if we don't have enough space, reallocate
                if x.len() == Self::inline_capacity() {
                    self.reallocate_to_heap();

                    match self {
                        Self::Inline(_) => unsafe { std::hint::unreachable_unchecked() },
                        Self::Heap(x) => x.push(c),
                    }
                } else {
                    x.push(c);
                }
            }

            Self::Heap(x) => {
                x.push(c);
            }
        };
    }

    #[inline]
    pub fn extend(&mut self, data: &str) {
        self.extend_bytes(data.as_bytes());
    }

    #[inline]
    pub fn extend_bytes(&mut self, data: &[u8]) {
        match self {
            Self::Inline(x) => {
                // if we don't have enough space, reallocate
                if x.len() + data.len() > Self::inline_capacity() {
                    self.reallocate_to_heap();

                    match self {
                        Self::Inline(_) => unsafe { std::hint::unreachable_unchecked() },
                        Self::Heap(x) => x.extend_bytes(data),
                    }
                } else {
                    x.extend_bytes(data);
                }
            }
            Self::Heap(x) => {
                x.extend_bytes(data);
            }
        }
    }

    #[inline]
    pub fn clear(&mut self) {
        match self {
            Self::Inline(x) => x.clear(),
            Self::Heap(x) => x.clear(),
        }
    }

    /// Reallocates the inline buffer to heap. Must not be called if heap is already in use.
    fn reallocate_to_heap(&mut self) {
        let hrepr = match self {
            Self::Inline(x) => HeapRepr::new(x.as_bytes()),
            Self::Heap(_) => unreachable!("called reallocate_to_heap on a heap FastString"),
        };

        *self = Self::Heap(hrepr);
    }
}

impl FastString {
    pub fn new(data: &str) -> Self {
        Self::from_buffer(data.as_bytes())
    }

    pub fn from_buffer(data: &[u8]) -> Self {
        Self {
            repr: if data.len() >= INLINE_CAP {
                StringRepr::new_with_heap(data)
            } else {
                StringRepr::new_with_inline(data)
            },
        }
    }

    pub const fn inline_capacity() -> usize {
        StringRepr::inline_capacity()
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.repr.capacity()
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.repr.len()
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.repr.is_empty()
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        self.repr.as_bytes()
    }

    /// Converts the string to a `&str`, returns an error if utf-8 validation fails
    #[inline]
    pub fn to_str(&self) -> Result<&str, Utf8Error> {
        std::str::from_utf8(self.as_bytes())
    }

    /// Converts the string to a `&str`, bypassing utf-8 validation checks
    #[inline]
    pub unsafe fn to_str_unchecked(&self) -> &str {
        std::str::from_utf8_unchecked(self.as_bytes())
    }

    /// Converts the string to a `&str`, returns "<invalid UTF-8 string>"" if utf-8 validation fails
    #[inline]
    pub fn try_to_str(&self) -> &str {
        self.to_str().unwrap_or("<invalid UTF-8 string>")
    }

    #[inline]
    pub fn try_to_string(&self) -> String {
        self.to_str().map_or_else(|_| "<invalid UTF-8 string>".to_owned(), ToOwned::to_owned)
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        self.repr.push(c);
    }

    #[inline]
    pub fn extend(&mut self, data: &str) {
        self.repr.extend(data);
    }

    #[inline]
    pub fn is_heap(&self) -> bool {
        !self.repr.is_inline()
    }

    #[inline]
    /// Compares this string to `other` in constant time
    pub fn constant_time_compare(&self, other: &str) -> bool {
        if self.len() != other.len() {
            return false;
        }

        let mut result = 0u8;

        self.as_bytes()
            .iter()
            .take(self.len())
            .zip(other.as_bytes().iter().take(other.len()))
            .for_each(|(c1, c2)| result |= c1 ^ c2);

        result == 0
    }

    #[inline]
    pub fn copy_from_str(&mut self, other: &str) {
        self.repr.clear();
        self.repr.extend_bytes(other.as_bytes());
    }
}

impl Default for FastString {
    fn default() -> Self {
        Self::new("")
    }
}

impl Clone for FastString {
    fn clone(&self) -> Self {
        Self { repr: self.repr.clone() }
    }

    fn clone_from(&mut self, source: &Self) {
        self.repr.clear();
        self.repr.extend_bytes(source.as_bytes());
    }
}

impl Deref for FastString {
    type Target = str;

    fn deref(&self) -> &Self::Target {
        self.try_to_str()
    }
}

impl Display for FastString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(self.try_to_str())
    }
}

impl PartialEq for FastString {
    fn eq(&self, other: &Self) -> bool {
        let me_len = self.len();
        let other_len = other.len();

        if me_len != other_len {
            return false;
        }

        self.as_bytes().iter().take(me_len).eq(other.as_bytes().iter().take(other_len))
    }
}

impl Eq for FastString {}

impl From<FastString> for String {
    fn from(value: FastString) -> Self {
        value.try_to_str().to_owned()
    }
}

impl From<String> for FastString {
    fn from(value: String) -> Self {
        Self::new(value.as_str())
    }
}

impl From<&str> for FastString {
    fn from(value: &str) -> Self {
        Self::new(value)
    }
}

impl<const N: usize> From<InlineString<N>> for FastString {
    fn from(value: InlineString<N>) -> Self {
        Self::from_buffer(value.as_bytes())
    }
}

/* esp */

impl Encodable for FastString {
    #[inline]
    fn encode(&self, buf: &mut crate::ByteBuffer) {
        let len = self.len();
        buf.write_length(len);
        buf.write_bytes(&self.as_bytes()[..len]);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        let len = self.len();
        buf.write_length(len);
        buf.write_bytes(&self.as_bytes()[..len]);
    }
}

impl Decodable for FastString {
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let len = buf.read_length_check::<u8>()?;

        let end_pos = buf.get_rpos() + len;
        let str = Self::from_buffer(&buf.as_bytes()[buf.get_rpos()..end_pos]);
        buf.set_rpos(end_pos);

        Ok(str)
    }
}

impl DynamicSize for FastString {
    fn encoded_size(&self) -> usize {
        size_of_types!(VarLength) + self.len()
    }
}
