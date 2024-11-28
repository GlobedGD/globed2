pub mod client;
pub mod server;

pub use client::*;
pub use server::*;

use crate::data::*;

pub trait Packet: PacketMetadata {}

// god i hate this
pub trait PacketMetadata {
    const PACKET_ID: u16;
    const ENCRYPTED: bool;
    const SHOULD_USE_TCP: bool;
    const NAME: &'static str;
}

#[derive(Encodable, Decodable, StaticSize)]
pub struct PacketHeader {
    pub packet_id: u16,
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

    pub const SIZE: usize = Self::ENCODED_SIZE;
}
