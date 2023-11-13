use crate::{
    bytebufferext::{decode_unimpl, empty_impl, encode_impl, ByteBufferExtWrite},
    data::{packets::packet, types::CryptoPublicKey},
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

empty_impl!(PingResponsePacket, Self { id: 0, player_count: 0 });

decode_unimpl!(PingResponsePacket);

/* CryptoHandshakeResponsePacket - 20001 */

packet!(CryptoHandshakeResponsePacket, 20001, false, {
    key: CryptoPublicKey,
});

encode_impl!(CryptoHandshakeResponsePacket, buf, self, {
    buf.write_value(&self.key);
});

empty_impl!(
    CryptoHandshakeResponsePacket,
    Self {
        key: CryptoPublicKey::empty()
    }
);

decode_unimpl!(CryptoHandshakeResponsePacket);

/* KeepaliveResponsePacket - 20002 */

packet!(KeepaliveResponsePacket, 20002, false, {
    player_count: u32
});

encode_impl!(KeepaliveResponsePacket, buf, self, {
    buf.write_u32(self.player_count);
});

empty_impl!(KeepaliveResponsePacket, Self { player_count: 0 });

decode_unimpl!(KeepaliveResponsePacket);

/* ServerDisconnectPacket - 20003 */

packet!(ServerDisconnectPacket, 20003, false, {
    message: String
});

encode_impl!(ServerDisconnectPacket, buf, self, {
    buf.write_string(&self.message);
});

empty_impl!(ServerDisconnectPacket, Self { message: "".to_string() });

decode_unimpl!(ServerDisconnectPacket);

/* LoggedInPacket - 20004 */

packet!(LoggedInPacket, 20004, true, {});

encode_impl!(LoggedInPacket, _buf, self, {});

empty_impl!(LoggedInPacket, Self {});

decode_unimpl!(LoggedInPacket);
