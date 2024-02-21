pub mod consts;
pub mod packets;
pub mod types;

/* re-export all important types, packets and macros */
pub use consts::*;
pub use esp::*;
pub use globed_derive::*;
pub use globed_shared::MAX_NAME_SIZE;
pub use packets::*;
pub use types::*;

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

impl<'a> ByteBufferExtRead2 for ByteReader<'a> {
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
        self.write_u32(upper_bound as u32);

        let written = fs(self);

        if written != upper_bound {
            let endpos = self.get_wpos();
            self.set_wpos(lenpos);
            self.write_u32(written as u32);
            self.set_wpos(endpos);
        }

        written
    }
}

impl<'a> ByteBufferExtWrite2 for FastByteBuffer<'a> {
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
        self.write_u32(upper_bound as u32);

        let written = fs(self);

        if written != upper_bound {
            let endpos = self.get_pos();
            self.set_pos(lenpos);
            self.write_u32(written as u32);
            self.set_pos(endpos);
        }

        written
    }
}
