pub mod client;
pub mod server;

use std::any::Any;

use crate::bytebufferext::{Decodable, Encodable};

use self::client::*;

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

            fn as_any(&self) -> &dyn std::any::Any {
                self
            }
        }

        impl crate::data::packets::PacketWithId for $packet_type {
            const PACKET_ID: crate::data::packets::PacketId = $packet_id;
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

use anyhow::anyhow;
use bytebuffer::ByteReader;
pub(crate) use empty_client_packet;
pub(crate) use empty_server_packet;
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
    ($typ:ty,$br:expr) => {{
        Ok(Box::new(<$typ>::decode_from_reader($br)?))
    }};
}

pub fn match_packet(packet_id: PacketId, data: &mut ByteReader<'_>) -> anyhow::Result<Box<dyn Packet>> {
    match packet_id {
        PingPacket::PACKET_ID => mpacket!(PingPacket, data),
        CryptoHandshakeStartPacket::PACKET_ID => mpacket!(CryptoHandshakeStartPacket, data),
        KeepalivePacket::PACKET_ID => mpacket!(KeepalivePacket, data),
        LoginPacket::PACKET_ID => mpacket!(LoginPacket, data),
        DisconnectPacket::PACKET_ID => mpacket!(DisconnectPacket, data),

        // game related
        SyncIconsPacket::PACKET_ID => mpacket!(SyncIconsPacket, data),
        RequestProfilesPacket::PACKET_ID => mpacket!(RequestProfilesPacket, data),
        LevelJoinPacket::PACKET_ID => mpacket!(LevelJoinPacket, data),
        LevelLeavePacket::PACKET_ID => mpacket!(LevelLeavePacket, data),
        PlayerDataPacket::PACKET_ID => mpacket!(PlayerDataPacket, data),

        VoicePacket::PACKET_ID => mpacket!(VoicePacket, data),
        _ => Err(anyhow!("no matching packet in 'match_packet' with id {packet_id}")),
    }
}
