use crate::data::types::cocos;
use anyhow::Result;
use bytebuffer::ByteBuffer;

type ByteVec = Vec<u8>;

pub trait Encodable {
    fn encode(&self, buf: &mut ByteBuffer);
}

pub trait Decodable {
    fn decode(buf: &mut ByteBuffer) -> Result<Self>
    where
        Self: Sized;
}

pub trait Serializable: Encodable + Decodable {}

macro_rules! encode_impl {
    ($packet_type:ty, $buf:ident, $self:ident, $decode:expr) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&$self, $buf: &mut bytebuffer::ByteBuffer) {
                $decode
            }
        }
    };
}

macro_rules! decode_impl {
    ($packet_type:ty, $buf:ident, $decode:expr) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode($buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self>
            where
                Self: Sized,
            {
                $decode
            }
        }
    };
}

macro_rules! encode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Encodable for $packet_type {
            fn encode(&self, _: &mut bytebuffer::ByteBuffer) {
                unimplemented!();
            }
        }
    };
}

macro_rules! decode_unimpl {
    ($packet_type:ty) => {
        impl crate::bytebufferext::Decodable for $packet_type {
            fn decode($buf: &mut bytebuffer::ByteBuffer) -> anyhow::Result<Self>
            where
                Self: Sized,
            {
                Err(anyhow::anyhow!("decoding unimplemented for this type"))
            }
        }
    };
}
pub(crate) use decode_impl;
pub(crate) use decode_unimpl;
pub(crate) use encode_impl;
pub(crate) use encode_unimpl;

pub trait ByteBufferExt {
    // read a byte vector, prefixed with 4 bytes indicating length
    fn read_byte_array(&mut self) -> Result<ByteVec>;
    // write a byte vector, prefixed with 4 bytes indicating length
    fn write_byte_array(&mut self, vec: &ByteVec);

    fn read_value<T: Decodable>(&mut self) -> Result<T>;
    fn write_value<T: Encodable>(&mut self, val: &T);

    fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>>;
    fn write_value_vec<T: Encodable>(&mut self, val: &[T]);

    fn read_color3(&mut self) -> Result<cocos::Color3B>;
    fn read_color4(&mut self) -> Result<cocos::Color4B>;
    fn read_point(&mut self) -> Result<cocos::Point>;

    fn write_color3(&mut self, val: cocos::Color3B);
    fn write_color4(&mut self, val: cocos::Color4B);
    fn write_point(&mut self, val: cocos::Point);
}

impl ByteBufferExt for ByteBuffer {
    fn read_byte_array(&mut self) -> Result<ByteVec> {
        let length = self.read_u32()? as usize;
        Ok(self.read_bytes(length)?)
    }

    fn write_byte_array(&mut self, vec: &ByteVec) {
        self.write_u32(vec.len() as u32);
        self.write_bytes(&vec);
    }

    fn read_value<T: Decodable>(&mut self) -> Result<T> {
        T::decode(self)
    }

    fn write_value<T: Encodable>(&mut self, val: &T) {
        val.encode(self);
    }

    fn read_value_vec<T: Decodable>(&mut self) -> Result<Vec<T>> {
        let mut out = Vec::new();
        let length = self.read_u32()? as usize;
        for _ in 0..length {
            out.push(T::decode(self)?);
        }

        Ok(out)
    }

    fn write_value_vec<T: Encodable>(&mut self, val: &[T]) {
        self.write_u32(val.len() as u32);
        for elem in val.iter() {
            elem.encode(self);
        }
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
