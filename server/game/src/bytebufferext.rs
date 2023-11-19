use std::io::Read;

use crate::data::types::cocos;
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};

type ByteVec = Vec<u8>;

pub trait Encodable {
    fn encode(&self, buf: &mut ByteBuffer);
}

// empty is similar to default but default does silly things i think
pub trait Empty {
    fn empty() -> Self
    where
        Self: Sized;
}

pub trait Decodable: Empty {
    fn decode(&mut self, buf: &mut ByteBuffer) -> Result<()>;
    fn decode_from_reader(&mut self, buf: &mut ByteReader) -> Result<()>;
}

pub trait Serializable: Encodable + Decodable {}

macro_rules! encode_impl {
    ($packet_type:ty, $buf:ident, $self:ident, $encode:expr) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&$self, $buf: &mut bytebuffer::ByteBuffer) {
                $encode
            }
        }
    };
}

macro_rules! decode_impl {
    ($packet_type:ty, $buf:ident, $self:ident, $decode:expr) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode(&mut $self, $buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<()> {
                $decode
            }

            fn decode_from_reader(&mut $self, $buf: &mut bytebuffer::ByteReader) -> anyhow::Result<()> {
                $decode
            }
        }
    };
}

macro_rules! empty_impl {
    ($typ:ty, $empty:expr) => {
        impl crate::bytebufferext::Empty for $typ {
            fn empty() -> Self
            where
                Self: Sized,
            {
                $empty
            }
        }
    };
}

macro_rules! encode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&self, _: &mut bytebuffer::ByteBuffer) {
                panic!(
                    "Tried to call {}::encode when Encodable was not implemented for this type",
                    stringify!($packet_type)
                );
            }
        }
    };
}

macro_rules! decode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode(&mut self, _: &mut bytebuffer::ByteBuffer) -> anyhow::Result<()> {
                Err(anyhow::anyhow!(
                    "decoding unimplemented for {}",
                    stringify!($packet_type)
                ))
            }

            fn decode_from_reader(&mut self, _: &mut bytebuffer::ByteReader) -> anyhow::Result<()> {
                Err(anyhow::anyhow!(
                    "decoding unimplemented for {}",
                    stringify!($packet_type)
                ))
            }
        }
    };
}
pub(crate) use decode_impl;
pub(crate) use decode_unimpl;
pub(crate) use empty_impl;
pub(crate) use encode_impl;
pub(crate) use encode_unimpl;

pub trait ByteBufferExtWrite {
    // write a byte vector, prefixed with 4 bytes indicating length
    fn write_byte_array(&mut self, vec: &ByteVec);

    fn write_value<T: Encodable>(&mut self, val: &T);

    fn write_value_vec<T: Encodable>(&mut self, val: &[T]);

    fn write_color3(&mut self, val: cocos::Color3B);
    fn write_color4(&mut self, val: cocos::Color4B);
    fn write_point(&mut self, val: cocos::Point);
}

pub trait ByteBufferExtRead {
    // read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> Result<ByteVec>;
    fn read_bytes_into(&mut self, dest: &mut [u8]) -> Result<()>;

    fn read_value<T: Decodable>(&mut self) -> Result<T>;

    fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>>;

    fn read_color3(&mut self) -> Result<cocos::Color3B>;
    fn read_color4(&mut self) -> Result<cocos::Color4B>;
    fn read_point(&mut self) -> Result<cocos::Point>;
}

impl ByteBufferExtWrite for ByteBuffer {
    fn write_byte_array(&mut self, vec: &ByteVec) {
        self.write_u32(vec.len() as u32);
        self.write_bytes(vec);
    }

    fn write_value<T: Encodable>(&mut self, val: &T) {
        val.encode(self);
    }

    fn write_value_vec<T: Encodable>(&mut self, val: &[T]) {
        self.write_u32(val.len() as u32);
        for elem in val.iter() {
            elem.encode(self);
        }
    }

    fn write_color3(&mut self, val: cocos::Color3B) {
        self.write_value(&val)
    }

    fn write_color4(&mut self, val: cocos::Color4B) {
        self.write_value(&val)
    }

    fn write_point(&mut self, val: cocos::Point) {
        self.write_value(&val)
    }
}

macro_rules! impl_extread {
    ($decode_fn:ident) => {
        fn read_byte_array(&mut self) -> Result<ByteVec> {
            let length = self.read_u32()? as usize;
            Ok(self.read_bytes(length)?)
        }

        fn read_bytes_into(&mut self, dest: &mut [u8]) -> Result<()> {
            self.flush_bits();

            if self.get_rpos() + dest.len() > self.len() {
                return Err(anyhow!("could not read enough bytes from buffer"));
            }

            self.read_exact(dest)?;

            Ok(())
        }

        fn read_value<T: Decodable>(&mut self) -> Result<T> {
            let mut value = T::empty();
            value.$decode_fn(self)?;
            Ok(value)
        }

        fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>> {
            let mut out = Vec::new();
            let length = self.read_u32()? as usize;
            for _ in 0..length {
                let mut value = T::empty();
                value.$decode_fn(self)?;
                out.push(value);
            }

            Ok(out)
        }

        fn read_color3(&mut self) -> Result<cocos::Color3B> {
            self.read_value()
        }

        fn read_color4(&mut self) -> Result<cocos::Color4B> {
            self.read_value()
        }

        fn read_point(&mut self) -> Result<cocos::Point> {
            self.read_value()
        }
    };
}

impl ByteBufferExtRead for ByteBuffer {
    impl_extread!(decode);
}

impl<'a> ByteBufferExtRead for ByteReader<'a> {
    impl_extread!(decode_from_reader);
}
