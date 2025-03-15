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
//!     fn translate_from(data: &mut esp::ByteReader<'_>, protocol: u16) -> PacketTranslationResult<Self> {
//!         match protocol {
//!             // note the invariant: `protocol` is never equal to `CURRENT_PROTOCOL`
//!             CURRENT_PROTOCOL => unreachable!(),
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
//!
//! There are also the `PartialTranslatable` and `PartialTranslatableEncodable` traits, used primarily for outgoing packets.
//!
//! ```rust
//! impl PartialTranslatable<v13::PingResponsePacket> for v_current::PingResponsePacket {
//!     fn translate_to(self) -> PacketTranslationResult<v13::PingResponsePacket> {
//!         Ok(v13::PingResponsePacket {
//!             // some fields ...
//!         })
//!     }
//! }
//!
//! impl PartialTranslatableEncodable for PingResponsePacket {
//!     fn translate_and_encode(&self, protocol: u16, buf: &mut ByteBuffer) -> PacketTranslationResult<()> {
//!         match protocol {
//!             13 => {
//!                 let translated = <Self as PartialTranslatableTo<v13::ServerNoticePacket>>::translate_to(self)?;
//!                 buf.write_value(&translated);
//!                 Ok(())
//!             }
//!
//!             _ => Err(PacketTranslationError::UnsupportedProtocol),
//!         }
//!     }
//! }
//! ```

use esp::{ByteBuffer, ByteReader, Decodable, DecodeResult, FastByteBuffer};

pub use crate::data::{CURRENT_PROTOCOL, Packet};

mod error;
mod impls;
pub use error::PacketTranslationError;

pub type PacketTranslationResult<T> = Result<T, PacketTranslationError>;

pub trait Translatable: Packet + Sized + Decodable {
    // Default implementation for translating packets, which just assumes the packet structure is the same.
    fn translate_from(data: &mut ByteReader<'_>, _protocol: u16) -> PacketTranslationResult<Self> {
        let packet = Self::decode_from_reader(data)?;

        Ok(packet)
    }
}

pub trait PartialTranslatableTo<OutP>: Packet + Sized {
    fn translate_to(self) -> PacketTranslationResult<OutP>;
}

pub trait PartialTranslatableEncodable: Packet + Sized {
    fn translate_and_encode(self, protocol: u16, buf: &mut ByteBuffer) -> PacketTranslationResult<()>;

    fn translate_and_encode_fast(self, _protocol: u16, _buf: &mut FastByteBuffer) -> PacketTranslationResult<()> {
        panic!("`translate_and_encode_fast` not implemented for this packet");
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
        // simply decode the packet if the protocol is the same / max
        if self.protocol == CURRENT_PROTOCOL || self.protocol == 0xffff {
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
