/* RemainderBytes - wrapper around Box<[u8]> that decodes with `buf.read_remaining_bytes()` and encodes with `buf.write_bytes()` */

use std::ops::Deref;

use crate::*;

#[derive(Clone)]
#[repr(transparent)]
pub struct RemainderBytes {
    data: Box<[u8]>,
}

encode_impl!(RemainderBytes, buf, self, {
    buf.write_bytes(&self.data);
});

decode_impl!(RemainderBytes, buf, {
    Ok(Self {
        data: buf.read_remaining_bytes()?.into(),
    })
});

dynamic_size_calc_impl!(RemainderBytes, self, self.data.len());

impl Deref for RemainderBytes {
    type Target = [u8];
    #[inline]
    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl From<Vec<u8>> for RemainderBytes {
    #[inline]
    fn from(value: Vec<u8>) -> Self {
        Self {
            data: value.into_boxed_slice(),
        }
    }
}

impl From<Box<[u8]>> for RemainderBytes {
    #[inline]
    fn from(value: Box<[u8]>) -> Self {
        Self { data: value }
    }
}
