use crate::{
    bytebufferext::*,
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

decode_unimpl!(PingResponsePacket);

size_calc_impl!(PingResponsePacket, size_of_types!(u32, u32));

/* CryptoHandshakeResponsePacket - 20001 */

packet!(CryptoHandshakeResponsePacket, 20001, false, {
    key: CryptoPublicKey,
});

encode_impl!(CryptoHandshakeResponsePacket, buf, self, {
    buf.write_value(&self.key);
});

decode_unimpl!(CryptoHandshakeResponsePacket);

size_calc_impl!(CryptoHandshakeResponsePacket, CryptoPublicKey::ENCODED_SIZE);

/* KeepaliveResponsePacket - 20002 */

packet!(KeepaliveResponsePacket, 20002, false, {
    player_count: u32
});

encode_impl!(KeepaliveResponsePacket, buf, self, {
    buf.write_u32(self.player_count);
});

decode_unimpl!(KeepaliveResponsePacket);

size_calc_impl!(KeepaliveResponsePacket, size_of_types!(u32));

/* ServerDisconnectPacket - 20003 */

packet!(ServerDisconnectPacket, 20003, false, {
    message: String
});

encode_impl!(ServerDisconnectPacket, buf, self, {
    buf.write_string(&self.message);
});

decode_unimpl!(ServerDisconnectPacket);

size_calc_impl!(ServerDisconnectPacket, MAX_ENCODED_STRING_SIZE);

/* LoggedInPacket - 20004 */

empty_server_packet!(LoggedInPacket, 20004);

/* LoginFailedPacket - 20005 */

packet!(LoginFailedPacket, 20005, false, {
    message: String
});

encode_impl!(LoginFailedPacket, buf, self, {
    buf.write_string(&self.message);
});

decode_unimpl!(LoginFailedPacket);

size_calc_impl!(LoginFailedPacket, MAX_ENCODED_STRING_SIZE);

/* ServerNoticePacket - 20006 */
// used to communicate a simple message to the user

packet!(ServerNoticePacket, 20006, true, {
    message: String
});

encode_impl!(ServerNoticePacket, buf, self, {
    buf.write_string(&self.message);
});

decode_unimpl!(ServerNoticePacket);

size_calc_impl!(ServerNoticePacket, MAX_ENCODED_STRING_SIZE);
