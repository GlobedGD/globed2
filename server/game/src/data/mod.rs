pub mod consts;

/* re-export all important types, packets and macros */
pub use consts::*;
pub use esp::*;
pub use globed_derive::*;
pub use globed_shared::MAX_NAME_SIZE;

pub use v_current::VERSION as CURRENT_PROTOCOL;
pub use v_current::packets;
pub use v_current::types;

pub use packets::*;
pub use types::*;

pub mod v13;
pub mod v14;

// change this to the latest version as needed
pub use v14 as v_current;

// our own extension

pub trait ByteBufferExtRead2 {
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader>;
}

pub trait ByteBufferExtWrite2 {
    fn write_packet_header<T: PacketMetadata>(&mut self);
    fn write_list_with<FLoop>(&mut self, upper_bound: usize, fs: FLoop) -> usize
    where
        FLoop: FnOnce(&mut Self) -> usize;
}

impl ByteBufferExtRead2 for ByteBuffer {
    #[inline]
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader> {
        self.read_value()
    }
}

impl ByteBufferExtRead2 for ByteReader<'_> {
    #[inline]
    fn read_packet_header(&mut self) -> DecodeResult<PacketHeader> {
        self.read_value()
    }
}

impl ByteBufferExtWrite2 for ByteBuffer {
    #[inline]
    fn write_packet_header<T: PacketMetadata>(&mut self) {
        self.write_value(&PacketHeader::from_packet::<T>());
    }

    #[inline]
    fn write_list_with<FLoop>(&mut self, upper_bound: usize, fs: FLoop) -> usize
    where
        FLoop: FnOnce(&mut Self) -> usize,
    {
        let lenpos = self.get_wpos();
        self.write_length(upper_bound);

        let written = fs(self);

        if written != upper_bound {
            let endpos = self.get_wpos();
            self.set_wpos(lenpos);
            self.write_length(written);
            self.set_wpos(endpos);
        }

        written
    }
}

impl ByteBufferExtWrite2 for FastByteBuffer<'_> {
    #[inline]
    fn write_packet_header<T: PacketMetadata>(&mut self) {
        self.write_value(&PacketHeader::from_packet::<T>());
    }

    #[inline]
    fn write_list_with<FLoop>(&mut self, upper_bound: usize, fs: FLoop) -> usize
    where
        FLoop: FnOnce(&mut Self) -> usize,
    {
        let lenpos = self.get_pos();
        self.write_length(upper_bound);

        let written = fs(self);

        if written != upper_bound {
            let endpos = self.get_pos();
            self.set_pos(lenpos);
            self.write_length(written);
            self.set_pos(endpos);
        }

        written
    }
}
