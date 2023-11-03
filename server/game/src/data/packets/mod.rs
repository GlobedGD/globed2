pub mod client;
pub mod server;

use crate::bytebufferext::{Decodable, Encodable};

type PacketId = u16;
const HEADER_LEN: usize = 3; // u16 + bool

/*
* for creating a new packet, the basic structure is either:
* 1. packet!(PacketName, id, enc, { struct body })
* 2. struct PacketName {}
*    packet!(PacketName, id, enc)
*
* followed by packet_encode! and packet_decode! or their _unimpl versions
*/

macro_rules! packet {
    ($packet_type:ty, $packet_id:expr, $encrypted:expr) => {
        impl Packet for $packet_type {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                $packet_id
            }

            fn get_encrypted(&self) -> bool {
                $encrypted
            }
        }
    };
    ($packet_type:ident, $packet_id:expr, $encrypted:expr, { $($field:ident: $field_type:ty),* $(,)? }) => {
        pub struct $packet_type {
            $($field: $field_type),*
        }

        impl Packet for $packet_type {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                $packet_id
            }

            fn get_encrypted(&self) -> bool {
                $encrypted
            }
        }
    };
}

pub(crate) use packet;

pub trait Packet: Encodable + Decodable {
    fn get_packet_id(&self) -> PacketId;
    fn get_encrypted(&self) -> bool;
}
