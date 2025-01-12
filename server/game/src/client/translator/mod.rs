//! This module contains the packet translator, which is used to translate packets between different protocol versions.
//!
//! Each packet has to implement the `Translatable` trait. If using the default implementation,
//! the translator will assume the structure of the packet is the same for all protocol versions.
//! If that is not true, the packet has to implement the `Translatable` trait manually,
//! and either return an error or implement the translation logic.
//!
//! Example:
//! ```rust
//!
//! impl Translatable for PingPacket {
//!     fn translate_from(data: &mut esp::ByteReader<'_>, protocol: u16) -> Result<Self, PacketTranslationError> {
//!         match protocol {
//!             // note the invariant: `protocol` is never equal to `CURRENT_PROTOCOL`
//!             CURRENT_PROTOCOL => unreachable()!
//!
//!             13 => {
//!                 let old_packet = v13::PingPacket::decode_from_reader(data)?;
//!                 Ok(PingPacket { ... })
//!             }
//!
//!             _ => Err(PacketTranslationError::UnsupportedProtocol),
//!         }
//!     }
//! }
//! ```

use esp::{ByteReader, Decodable, DecodeResult};

pub use crate::data::{CURRENT_PROTOCOL, Packet};

mod error;
mod impls;
pub use error::PacketTranslationError;

pub trait Translatable: Packet + Sized + Decodable {
    // Default implementation for translating packets, which just assumes the packet structure is the same.
    fn translate_from(data: &mut ByteReader<'_>, _protocol: u16) -> Result<Self, PacketTranslationError> {
        let packet = Self::decode_from_reader(data)?;

        Ok(packet)
    }
}

pub struct PacketTranslator {
    pub protocol: u16,
}

impl PacketTranslator {
    pub fn new(protocol: u16) -> Self {
        Self { protocol }
    }

    pub fn translate_packet<P>(&self, data: &mut ByteReader<'_>) -> Result<DecodeResult<P>, PacketTranslationError>
    where
        P: Packet + Decodable + Translatable,
    {
        // simply decode the packet if the protocol is the same
        if self.protocol == CURRENT_PROTOCOL {
            return Ok(P::decode_from_reader(data));
        }

        let translated = P::translate_from(data, self.protocol)?;

        Ok(Ok(translated))
    }
}

#[derive(Default)]
pub struct NoopPacketTranslator {}

impl NoopPacketTranslator {
    pub fn translate_packet<P: Packet + Decodable>(&self, data: &mut ByteReader<'_>) -> Result<esp::DecodeResult<P>, PacketTranslationError> {
        Ok(P::decode_from_reader(data))
    }
}
