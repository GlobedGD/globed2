pub mod client;
pub mod server;

pub use client::*;
pub use server::*;

use crate::data::bytebufferext::*;

type PacketId = u16;

pub trait Packet: Encodable + Decodable + Send + Sync + PacketMetadata {
    fn get_packet_id(&self) -> PacketId;
    fn get_encrypted(&self) -> bool;
}

// god i hate this
pub trait PacketMetadata {
    const PACKET_ID: PacketId;
    const ENCRYPTED: bool;
    const NAME: &'static str;
}

pub struct PacketHeader {
    pub packet_id: PacketId,
    pub encrypted: bool,
}

impl PacketHeader {
    #[inline]
    pub const fn from_packet<P: PacketMetadata>() -> Self {
        Self {
            packet_id: P::PACKET_ID,
            encrypted: P::ENCRYPTED,
        }
    }

    pub const SIZE: usize = size_of_types!(PacketId, bool);
}

encode_impl!(PacketHeader, buf, self, {
    buf.write_u16(self.packet_id);
    buf.write_bool(self.encrypted);
});

decode_impl!(PacketHeader, buf, {
    let packet_id = buf.read_u16()?;
    let encrypted = buf.read_bool()?;
    Ok(Self { packet_id, encrypted })
});

size_calc_impl!(PacketHeader, PacketHeader::SIZE);
