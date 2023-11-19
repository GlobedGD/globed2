use crate::{
    bytebufferext::{decode_unimpl, empty_impl, encode_impl, ByteBufferExtWrite},
    data::{
        packets::{empty_server_packet, packet},
        types::CryptoPublicKey,
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

empty_server_packet!(LoggedInPacket, 20004);

/* LoginFailedPacket - 20005 */

packet!(LoginFailedPacket, 20005, false, {
    message: String
});

encode_impl!(LoginFailedPacket, buf, self, {
    buf.write_string(&self.message);
});

empty_impl!(LoginFailedPacket, Self { message: "".to_string() });

decode_unimpl!(LoginFailedPacket);

/* ServerNoticePacket - 20006 */
// used to communicate a simple message to the user

packet!(ServerNoticePacket, 20006, true, {
    message: String
});

encode_impl!(ServerNoticePacket, buf, self, {
    buf.write_string(&self.message);
});

empty_impl!(ServerNoticePacket, Self { message: "".to_string() });

decode_unimpl!(ServerNoticePacket);
