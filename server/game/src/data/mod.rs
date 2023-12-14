pub mod consts;
pub mod packets;
pub mod types;

/* re-export all important types, packets and macros */
pub use consts::*;
pub use esp::*;
pub use globed_derive::*;
pub use packets::*;
pub use types::*;

// our own extension

pub trait ByteBufferExtRead2 {
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader>;
}

pub trait ByteBufferExtWrite2 {
    fn write_packet_header<T: PacketMetadata>(&mut self);
}

impl ByteBufferExtRead2 for ByteBuffer {
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader> {
        self.read_value()
    }
}

impl<'a> ByteBufferExtRead2 for ByteReader<'a> {
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader> {
        self.read_value()
    }
}

impl ByteBufferExtWrite2 for ByteBuffer {
    fn write_packet_header<T: PacketMetadata>(&mut self) {
        self.write_value(&PacketHeader::from_packet::<T>());
    }
}

impl<'a> ByteBufferExtWrite2 for FastByteBuffer<'a> {
    fn write_packet_header<T: PacketMetadata>(&mut self) {
        self.write_value(&PacketHeader::from_packet::<T>());
    }
}
