pub mod client;
pub mod server;

pub use client::*;
pub use server::*;

use crate::data::bytebufferext::*;

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

        impl crate::data::packets::PacketMetadata for $packet_type {
            const PACKET_ID: crate::data::packets::PacketId = $packet_id;
            const ENCRYPTED: bool = $encrypted;
            const NAME: &'static str = stringify!($packet_type);
        }

        impl $packet_type {
            pub const fn header() -> crate::data::packets::PacketHeader {
                crate::data::packets::PacketHeader::from_packet::<Self>()
            }
        }
    };

    ($packet_type:ident, $packet_id:expr, $encrypted:expr, { $($field:ident: $field_type:ty),* $(,)? }) => {
        #[derive(Clone)]
        pub struct $packet_type {
            $(pub $field: $field_type),*
        }

        packet!($packet_type, $packet_id, $encrypted);
    };
}

macro_rules! empty_server_packet {
    ($packet_type:ident, $packet_id:expr) => {
        packet!($packet_type, $packet_id, false, {});

        encode_impl!($packet_type, _buf, self, {});

        decode_unimpl!($packet_type);

        size_calc_impl!($packet_type, 0);
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
