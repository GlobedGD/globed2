use crate::data::*;

/* PingPacket - 10000 */

packet!(PingPacket, 10000, false, {
    id: u32,
});

encode_unimpl!(PingPacket);

decode_impl!(PingPacket, buf, Ok(Self { id: buf.read_u32()? }));

/* CryptoHandshakeStartPacket - 10001 */

packet!(CryptoHandshakeStartPacket, 10001, false, {
    protocol: u16,
    key: CryptoPublicKey,
});

encode_unimpl!(CryptoHandshakeStartPacket);

decode_impl!(CryptoHandshakeStartPacket, buf, {
    let protocol = buf.read_u16()?;
    let key = buf.read_value()?;
    Ok(Self { protocol, key })
});

/* KeepalivePacket - 10002 */

empty_client_packet!(KeepalivePacket, 10002);

/* LoginPacket - 10003 */

packet!(LoginPacket, 10003, true, {
    account_id: i32,
    token: String,
});

encode_unimpl!(LoginPacket);

decode_impl!(LoginPacket, buf, {
    let account_id = buf.read_i32()?;
    let token = buf.read_string()?;
    Ok(Self { account_id, token })
});

/* DisconnectPacket - 10004 */

empty_client_packet!(DisconnectPacket, 10004);
