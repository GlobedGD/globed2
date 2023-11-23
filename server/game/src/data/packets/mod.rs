pub mod client;
pub mod server;

use crate::bytebufferext::{Decodable, Encodable};

type PacketId = u16;

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
        impl crate::data::packets::Packet for $packet_type {
            fn get_packet_id(&self) -> crate::data::packets::PacketId {
                $packet_id
            }

            fn get_encrypted(&self) -> bool {
                $encrypted
            }
        }

        impl crate::data::packets::PacketWithId for $packet_type {
            const PACKET_ID: crate::data::packets::PacketId = $packet_id;
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
        }

        impl crate::data::packets::PacketMetadata for $packet_type {
            const PACKET_ID: crate::data::packets::PacketId = $packet_id;
            const ENCRYPTED: bool = $encrypted;
        }
    };
}

macro_rules! empty_server_packet {
    ($packet_type:ident, $packet_id:expr) => {
        packet!($packet_type, $packet_id, false, {});

        encode_impl!($packet_type, _buf, self, {});

        decode_unimpl!($packet_type);
    };
}

macro_rules! empty_client_packet {
    ($packet_type:ident, $packet_id:expr) => {
        packet!($packet_type, $packet_id, false, {});

        encode_unimpl!($packet_type);

        decode_impl!($packet_type, _buf, Ok(Self {}));
    };
}

pub(crate) use empty_client_packet;
pub(crate) use empty_server_packet;
pub(crate) use packet;

pub trait Packet: Encodable + Decodable + Send + Sync + PacketMetadata {
    fn get_packet_id(&self) -> PacketId;
    fn get_encrypted(&self) -> bool;
}

// god i hate this
pub trait PacketMetadata {
    const PACKET_ID: PacketId;
    const ENCRYPTED: bool;
}

pub const PACKET_HEADER_LEN: usize = std::mem::size_of::<PacketId>() + std::mem::size_of::<bool>();
