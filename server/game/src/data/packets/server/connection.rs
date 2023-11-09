use crate::{
    bytebufferext::{decode_unimpl, empty_impl, encode_impl, ByteBufferExtWrite},
    data::{
        packets::{packet, Packet},
        types::handshake_data::HandshakeResponseData,
    },
};

/* PingResponsePacket - 20000 */

packet!(PingResponsePacket, 20000, false, {
    id: u32,
    player_count: u32
});

encode_impl!(PingResponsePacket, buf, self, {
    buf.write_u32(self.id);
    buf.write_u32(self.player_count);
});

empty_impl!(
    PingResponsePacket,
    Self {
        id: 0,
        player_count: 0
    }
);

decode_unimpl!(PingResponsePacket);

/* CryptoHandshakeResponsePacket - 20001 */

packet!(CryptoHandshakeResponsePacket, 20001, false, {
    data: HandshakeResponseData,
});

encode_impl!(CryptoHandshakeResponsePacket, buf, self, {
    buf.write_value(&self.data);
});

empty_impl!(
    CryptoHandshakeResponsePacket,
    Self {
        data: HandshakeResponseData::empty()
    }
);

decode_unimpl!(CryptoHandshakeResponsePacket);
