use crate::{
    bytebufferext::{decode_impl, empty_impl, encode_unimpl, ByteBufferExtRead},
    data::{
        packets::{packet, Packet},
        types::handshake_data::HandshakeData,
    },
};

/* PingPacket - 10000 */

packet!(PingPacket, 10000, false, {
    id: u32,
});

empty_impl!(PingPacket, Self { id: 0 });

encode_unimpl!(PingPacket);

decode_impl!(PingPacket, buf, self, {
    self.id = buf.read_u32()?;
    Ok(())
});

/* CryptoHandshakeStartPacket - 10001 */

packet!(CryptoHandshakeStartPacket, 10001, false, {
    data: HandshakeData,
});

encode_unimpl!(CryptoHandshakeStartPacket);

empty_impl!(CryptoHandshakeStartPacket, {
    Self {
        data: HandshakeData::empty(),
    }
});

decode_impl!(CryptoHandshakeStartPacket, buf, self, {
    self.data = buf.read_value()?;
    Ok(())
});
