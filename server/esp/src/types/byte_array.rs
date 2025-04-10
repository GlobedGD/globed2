use std::ops::Deref;

use crate::*;

/// `ByteArray<N>` is a simple wrapper around [u8; N] that can be easily encoded/decoded with esp.
#[derive(Clone)]
pub struct ByteArray<const N: usize> {
    storage: [u8; N],
}

impl<const N: usize> ByteArray<N> {
    pub fn new(storage: [u8; N]) -> Self {
        Self { storage }
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.storage[..N]
    }
}

impl<const N: usize> Default for ByteArray<N> {
    fn default() -> Self {
        Self::new([0u8; N])
    }
}

impl<const N: usize> Deref for ByteArray<N> {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        self.as_bytes()
    }
}

/* esp */

impl<const N: usize> Encodable for ByteArray<N> {
    #[inline]
    fn encode(&self, buf: &mut crate::ByteBuffer) {
        buf.write_bytes(&self.storage[..N]);
    }

    #[inline]
    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_bytes(&self.storage[..N]);
    }
}

impl<const N: usize> Decodable for ByteArray<N> {
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        buf.read_exact_bytes::<N>().map(Self::new)
    }
}

impl<const N: usize> StaticSize for ByteArray<N> {
    const ENCODED_SIZE: usize = N;
}

impl<const N: usize> DynamicSize for ByteArray<N> {
    fn encoded_size(&self) -> usize {
        Self::ENCODED_SIZE
    }
}
