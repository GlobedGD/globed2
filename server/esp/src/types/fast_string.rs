use std::mem::ManuallyDrop;
use std::ops::Deref;
use std::str::Utf8Error;

use crate::*;

/// `FastString` is an SSO string class, that can store up to `FastString::inline_capacity()` bytes inline,
/// and otherwise requires a heap allocation.
///
/// Just like `InlineString`, it can be converted to and from a `String`.
#[derive(Clone)]
pub struct FastString {
    repr: StringRepr,
}

const INLINE_CAP: usize = 64; // including null terminator

union InnerStringRepr {
    inline: InlineRepr,
    heap: ManuallyDrop<HeapRepr>,
}

struct StringRepr {
    inner: InnerStringRepr,
    is_inline: bool,
}

#[derive(Copy, Clone, Debug)]
struct InlineRepr {
    buf: [u8; INLINE_CAP],
}

#[derive(Debug)]
struct HeapRepr {
    data: Vec<u8>,
}

impl InlineRepr {
    #[inline]
    pub fn len(&self) -> usize {
        let mut idx = 0;
        for c in self.buf {
            if c == 0u8 {
                break;
            }

            idx += 1;
        }

        idx
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.buf[0] == 0u8
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.buf.as_ptr(), self.len()) }
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        let idx = self.len();
        self.buf[idx] = c;
        self.buf[idx + 1] = 0u8;
    }

    #[inline]
    pub fn extend(&mut self, data: &str) {
        let idx = self.len();

        debug_assert!(idx + data.len() < INLINE_CAP);

        unsafe { std::ptr::copy_nonoverlapping(data.as_ptr(), self.buf[idx..].as_mut_ptr(), data.len()) }

        self.buf[idx + data.len()] = 0u8;
    }
}

impl HeapRepr {
    #[inline]
    fn new(data: &[u8]) -> Self {
        let len = data.len();
        // thats a fucking thing?? i love you rust
        let cap = len.next_power_of_two();

        let mut buf = Vec::with_capacity(cap);

        unsafe {
            std::ptr::copy_nonoverlapping(data.as_ptr(), buf.as_mut_ptr(), len);
            buf.set_len(len);
        }

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
    pub fn extend(&mut self, data: &str) {
        self.data.extend(data.as_bytes());
    }
}

impl StringRepr {
    #[inline]
    fn is_inline(&self) -> bool {
        self.is_inline
    }

    pub fn new(data: &[u8]) -> Self {
        if data.len() >= INLINE_CAP {
            Self::new_with_heap(data)
        } else {
            Self::new_with_inline(data)
        }
    }

    #[inline]
    pub fn new_with_heap(data: &[u8]) -> Self {
        Self {
            inner: InnerStringRepr {
                heap: ManuallyDrop::new(HeapRepr::new(data)),
            },
            is_inline: false,
        }
    }

    #[inline]
    fn new_with_inline(data: &[u8]) -> Self {
        let len = data.len();

        debug_assert!(len < INLINE_CAP);

        let mut buf = [0u8; INLINE_CAP];

        unsafe {
            std::ptr::copy_nonoverlapping(data.as_ptr(), buf.as_mut_ptr(), len);
        }

        buf[len] = 0u8;

        Self {
            inner: InnerStringRepr {
                inline: InlineRepr { buf },
            },
            is_inline: true,
        }
    }

    pub const fn inline_capacity() -> usize {
        INLINE_CAP - 1
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        if self.is_inline() {
            Self::inline_capacity()
        } else {
            unsafe { self.inner.heap.capacity() }
        }
    }

    #[inline]
    pub fn len(&self) -> usize {
        if self.is_inline() {
            unsafe { self.inner.inline.len() }
        } else {
            unsafe { self.inner.heap.len() }
        }
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        if self.is_inline() {
            unsafe { self.inner.inline.is_empty() }
        } else {
            unsafe { self.inner.heap.len() == 0 }
        }
    }

    #[inline]
    pub fn as_bytes(&self) -> &[u8] {
        if self.is_inline() {
            unsafe { self.inner.inline.as_bytes() }
        } else {
            unsafe { self.inner.heap.as_bytes() }
        }
    }

    #[inline]
    pub fn push(&mut self, c: u8) {
        if self.is_inline() {
            // if we don't have enough space, reallocate
            if self.len() == Self::inline_capacity() {
                unsafe {
                    self.reallocate_to_heap();
                    (*self.inner.heap).push(c);
                }
            } else {
                unsafe {
                    self.inner.inline.push(c);
                }
            }
        } else {
            unsafe { (*self.inner.heap).push(c) }
        }
    }

    #[inline]
    pub fn extend(&mut self, data: &str) {
        if self.is_inline() {
            if self.len() + data.len() > Self::inline_capacity() {
                unsafe {
                    self.reallocate_to_heap();
                    (*self.inner.heap).extend(data);
                }
            } else {
                unsafe { self.inner.inline.extend(data) }
            }
        } else {
            unsafe { (*self.inner.heap).extend(data) }
        }
    }

    /// Reallocates the inline buffer to heap. Must not be called if heap is already in use.
    unsafe fn reallocate_to_heap(&mut self) {
        debug_assert!(self.is_inline());

        self.inner.heap = ManuallyDrop::new(HeapRepr::new(self.inner.inline.as_bytes()));
        self.is_inline = false;
    }
}

impl Drop for StringRepr {
    fn drop(&mut self) {
        if !self.is_inline() {
            unsafe {
                let _val = ManuallyDrop::<HeapRepr>::take(&mut self.inner.heap);
                // drop it
            }
        }
    }
}

impl Clone for StringRepr {
    fn clone(&self) -> Self {
        if self.is_inline() {
            Self {
                inner: InnerStringRepr {
                    inline: unsafe { self.inner.inline },
                },
                is_inline: true,
            }
        } else {
            unsafe { Self::new(self.inner.heap.as_bytes()) }
        }
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
        self.to_str()
            .map_or_else(|_| "<invalid UTF-8 string>".to_owned(), ToOwned::to_owned)
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
}

impl Default for FastString {
    fn default() -> Self {
        Self::new("")
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

        self.as_bytes()
            .iter()
            .take(me_len)
            .eq(other.as_bytes().iter().take(other_len))
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

/* esp */

impl Encodable for FastString {
    #[inline]
    fn encode(&self, buf: &mut crate::ByteBuffer) {
        let len = self.len();
        buf.write_u32(len as u32);
        buf.write_bytes(&self.as_bytes()[..len]);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        let len = self.len();
        buf.write_u32(len as u32);
        buf.write_bytes(&self.as_bytes()[..len]);
    }
}

impl Decodable for FastString {
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let len = buf.read_length()?;

        if len < INLINE_CAP {
            let mut buffer = [0u8; INLINE_CAP];
            std::io::Read::read(buf, &mut buffer[..len])?;

            Ok(Self::from_buffer(&buffer[..len]))
        } else {
            let slc = &buf.as_bytes()[buf.get_rpos()..buf.get_rpos() + len];

            Ok(Self::from_buffer(slc))
        }
    }
}

impl DynamicSize for FastString {
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.len()
    }
}
