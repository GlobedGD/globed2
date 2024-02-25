use std::{mem::MaybeUninit, ops::Deref};

use crate::*;

pub struct FastVec<T, const N: usize> {
    data: [MaybeUninit<T>; N],
    length: usize,
}

#[derive(Debug)]
pub struct FastVecOverflowError;

impl<T, const N: usize> FastVec<T, N> {
    pub fn new() -> Self {
        FastVec {
            data: unsafe { MaybeUninit::uninit().assume_init() },
            length: 0,
        }
    }

    #[inline]
    pub const fn len(&self) -> usize {
        self.length
    }

    #[inline]
    pub const fn is_empty(&self) -> bool {
        self.length == 0
    }

    #[inline]
    pub const fn capacity(&self) -> usize {
        N
    }

    #[inline]
    pub fn push(&mut self, value: T) {
        debug_assert!(
            self.len() < self.capacity(),
            "Attempted to push too many elements into a FastVec (length is at {} when capacity is {})",
            self.len(),
            self.capacity()
        );

        self.data[self.length] = MaybeUninit::new(value);
        self.length += 1;
    }

    #[inline]
    pub fn safe_push(&mut self, value: T) -> Result<(), FastVecOverflowError> {
        if self.len() >= self.capacity() {
            return Err(FastVecOverflowError);
        }

        self.push(value);
        Ok(())
    }
}

impl<T, const N: usize> Drop for FastVec<T, N> {
    fn drop(&mut self) {
        // free all of the elements in the vector.
        for elem in &mut self.data[..self.length] {
            unsafe {
                // safety: we know that all elements until `self.length` are initialized.
                elem.as_mut_ptr().drop_in_place();
            }
        }
    }
}

impl<T, const N: usize> Default for FastVec<T, N> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T, const N: usize> Iterator for FastVec<T, N> {
    type Item = T;

    fn next(&mut self) -> Option<Self::Item> {
        // this is black magic imo
        if self.length > 0 {
            self.length -= 1;
            Some(unsafe { self.data[self.length].as_ptr().read() })
        } else {
            None
        }
    }
}

impl<T, const N: usize> Deref for FastVec<T, N> {
    type Target = [T];

    fn deref(&self) -> &[T] {
        // safety: we know that all elements up until `self.length` are initialized.
        // also fucking hell this code looks scary
        unsafe { &*(std::ptr::from_ref::<[std::mem::MaybeUninit<T>]>(&self.data[..self.length]) as *const [T]) }
    }
}

impl<T, const N: usize> DynamicSize for FastVec<T, N>
where
    T: DynamicSize,
{
    fn encoded_size(&self) -> usize {
        size_of_types!(u32) + self.iter().map(T::encoded_size).sum::<usize>()
    }
}

impl<T, const N: usize> Encodable for FastVec<T, N>
where
    T: Encodable,
{
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_value_vec(self);
    }

    fn encode_fast(&self, buf: &mut FastByteBuffer) {
        buf.write_value_vec(self);
    }
}

impl<T, const N: usize> Decodable for FastVec<T, N>
where
    T: Decodable,
{
    fn decode_from_reader(buf: &mut ByteReader) -> DecodeResult<Self>
    where
        Self: Sized,
    {
        let size = buf.read_u32()? as usize;
        if size > N {
            return Err(DecodeError::NotEnoughCapacity);
        }

        let mut vec = Self::new();
        for _ in 0..size {
            vec.push(buf.read_value()?);
        }

        Ok(vec)
    }
}
