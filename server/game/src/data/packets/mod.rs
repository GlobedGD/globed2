pub mod client;
pub mod server;

use std::any::Any;

use crate::bytebufferext::{Decodable, Empty, Encodable};

use self::client::{CryptoHandshakeStartPacket, PingPacket};

type PacketId = u16;

/*
* for creating a new packet, the basic structure is either:
* 1. packet!(PacketName, id, enc, { struct body })
* 2. struct PacketName {}
*    packet!(PacketName, id, enc)
*
* followed by packet_encode! and packet_decode! or their _unimpl versions
* the empty_impl! also must be added which is basically Default but less silly i guess?
*/

macro_rules! packet {
    ($packet_type:ty, $packet_id:expr, $encrypted:expr) => {
        impl crate::data::packets::Packet for $packet_type {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                $packet_id
            }

            fn get_encrypted(&self) -> bool {
                $encrypted
            }

            fn as_any(&self) -> &dyn std::any::Any {
                self
            }

            impl crate::data::packets::PacketWithId for $packet_type {
                const PACKET_ID: crate::data::packets::PacketId = $packet_id;
            }
        }
    };
    ($packet_type:ident, $packet_id:expr, $encrypted:expr, { $($field:ident: $field_type:ty),* $(,)? }) => {
        #[derive(Clone)]
        pub struct $packet_type {
            $(pub $field: $field_type),*
        }

        impl crate::data::packets::Packet for $packet_type {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                $packet_id
            }

            fn get_encrypted(&self) -> bool {
                $encrypted
            }

            fn as_any(&self) -> &dyn std::any::Any {
                self
            }
        }

        impl crate::data::packets::PacketWithId for $packet_type {
            const PACKET_ID: crate::data::packets::PacketId = $packet_id;
        }
    };
}

pub(crate) use packet;

pub trait Packet: Encodable + Decodable + Send + Sync {
    fn get_packet_id(&self) -> PacketId;
    fn get_encrypted(&self) -> bool;
    fn as_any(&self) -> &dyn Any;
}

// god i hate this
pub trait PacketWithId {
    const PACKET_ID: PacketId;
}

pub const PACKET_HEADER_LEN: usize = std::mem::size_of::<PacketId>() + std::mem::size_of::<bool>();

macro_rules! mpacket {
    ($typ:ty) => {{
        Some(Box::new(<$typ>::empty()))
    }};
}

pub fn match_packet(packet_id: PacketId) -> Option<Box<dyn Packet>> {
    match packet_id {
        10000 => mpacket!(PingPacket),
        10001 => mpacket!(CryptoHandshakeStartPacket),
        _ => None,
    }
}
