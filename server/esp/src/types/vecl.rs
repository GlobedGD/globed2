use std::ops::Deref;

use crate::*;

#[derive(Debug, Clone, Default)]
pub struct Vec1L<T> {
    vec: Vec<T>,
}

impl<T> Vec1L<T> {
    pub fn new() -> Self {
        Self { vec: Vec::new() }
    }
}

impl<T> Deref for Vec1L<T> {
    type Target = Vec<T>;

    fn deref(&self) -> &Vec<T> {
        &self.vec
    }
}

impl<T: Encodable> Encodable for Vec1L<T> {
    fn encode(&self, buf: &mut ByteBuffer) {
        assert!(self.len() <= u8::MAX as usize, "Vec1L overflow when encoding");
        buf.write_u8(self.len() as u8);

        for elem in &**self {
            buf.write_value(elem);
        }
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        assert!(self.len() <= u8::MAX as usize, "Vec1L overflow when encoding");
        buf.write_u8(self.len() as u8);

        for elem in &**self {
            buf.write_value(elem);
        }
    }
}

impl<T: Decodable> Decodable for Vec1L<T> {
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let size = buf.read_u8()?;

        let mut vec = Vec::with_capacity(size as usize);

        for _ in 0..size {
            vec.push(buf.read_value()?);
        }

        Ok(Self { vec })
    }
}

impl<T: DynamicSize> DynamicSize for Vec1L<T> {
    fn encoded_size(&self) -> usize {
        size_of_types!(u8) + self.iter().map(T::encoded_size).sum::<usize>()
    }
}

// Vec4L

#[derive(Debug, Clone, Default)]
pub struct Vec4L<T> {
    vec: Vec<T>,
}

impl<T> Vec4L<T> {
    pub fn new() -> Self {
        Self { vec: Vec::new() }
    }
}

impl<T> Deref for Vec4L<T> {
    type Target = Vec<T>;

    fn deref(&self) -> &Vec<T> {
        &self.vec
    }
}

impl<T: Encodable> Encodable for Vec4L<T> {
    fn encode(&self, buf: &mut ByteBuffer) {
        assert!(self.len() <= u32::MAX as usize, "Vec4L overflow when encoding");
        buf.write_u32(self.len() as u32);

        for elem in &**self {
            buf.write_value(elem);
        }
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        assert!(self.len() <= u32::MAX as usize, "Vec1L overflow when encoding");
        buf.write_u32(self.len() as u32);

        for elem in &**self {
            buf.write_value(elem);
        }
    }
}

impl<T: Decodable> Decodable for Vec4L<T> {
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let size = buf.read_u32()?;

        let mut vec = Vec::with_capacity(size as usize);

        for _ in 0..size {
            vec.push(buf.read_value()?);
        }

        Ok(Self { vec })
    }
}

impl<T: DynamicSize> DynamicSize for Vec4L<T> {
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.iter().map(T::encoded_size).sum::<usize>()
    }
}
