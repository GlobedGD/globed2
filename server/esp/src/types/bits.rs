use crate::*;

// N is the number of bytes, not bits.
#[derive(Clone, Copy, Debug)]
pub struct Bits<const N: usize> {
    buffer: [u8; N],
}

impl<const N: usize> Bits<N> {
    pub fn new() -> Self {
        Self { buffer: [0u8; N] }
    }

    #[inline]
    pub fn reset(&mut self) {
        for i in 0..N {
            self.buffer[i] = 0;
        }
    }

    #[inline]
    pub fn set_bit(&mut self, pos: usize) {
        self.assign_bit(pos, true);
    }

    #[inline]
    pub fn clear_bit(&mut self, pos: usize) {
        self.assign_bit(pos, false);
    }

    #[inline]
    pub fn get_bit(&self, pos: usize) -> bool {
        let byte = pos / 8;
        let bit = 7 - (pos % 8);

        self.buffer[byte] & (1 << bit) > 0
    }

    #[inline]
    pub fn assign_bit(&mut self, pos: usize, state: bool) {
        debug_assert!(
            pos < N * 8,
            "attempting to assign bit number {pos} in a bit buffer with length of {} bits",
            N * 8
        );

        let byte = pos / 8;
        let bit = 7 - (pos % 8);

        if state {
            // set bit
            self.buffer[byte] |= 1 << bit;
        } else {
            // clear bit
            self.buffer[byte] &= !(1 << bit);
        }
    }
}

impl<const N: usize> Default for Bits<N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<const N: usize> Encodable for Bits<N> {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_array(&self.buffer);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_array(&self.buffer);
    }
}

impl<const N: usize> Decodable for Bits<N> {
    fn decode(buf: &mut ByteBuffer) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(Self {
            buffer: buf.read_value_array()?,
        })
    }

    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        Ok(Self {
            buffer: buf.read_value_array()?,
        })
    }
}

impl<const N: usize> StaticSize for Bits<N> {
    const ENCODED_SIZE: usize = N;
}

impl<const N: usize> DynamicSize for Bits<N> {
    fn encoded_size(&self) -> usize {
        Self::ENCODED_SIZE
    }
}
