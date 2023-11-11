use crate::bytebufferext::*;
use crate::data::packets::*;
use crate::data::types::crypto::CryptoPublicKey;

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
    account_id: i32,
    token: String,
    key: CryptoPublicKey,
});

encode_unimpl!(CryptoHandshakeStartPacket);

empty_impl!(CryptoHandshakeStartPacket, {
    Self {
        account_id: 0,
        token: "".to_string(),
        key: CryptoPublicKey::empty(),
    }
});

decode_impl!(CryptoHandshakeStartPacket, buf, self, {
    self.account_id = buf.read_i32()?;
    self.token = buf.read_string()?;
    self.key = buf.read_value()?;
    Ok(())
});

/* KeepalivePacket - 10002 */

packet!(KeepalivePacket, 10002, false, {});

encode_unimpl!(KeepalivePacket);

empty_impl!(KeepalivePacket, Self {});

decode_impl!(KeepalivePacket, _buf, self, Ok(()));
