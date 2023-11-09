use anyhow::anyhow;
use crypto_box::{PublicKey, KEY_SIZE};

use crate::bytebufferext::{decode_impl, decode_unimpl, empty_impl, encode_impl, encode_unimpl};

#[derive(Clone)]
pub struct HandshakeData {
    pub pubkey: PublicKey,
}

#[derive(Clone)]
pub struct HandshakeResponseData {
    pub pubkey: PublicKey,
}

encode_unimpl!(HandshakeData);

empty_impl!(
    HandshakeData,
    Self {
        pubkey: PublicKey::from_bytes([0u8; 32])
    }
);

decode_impl!(HandshakeData, buf, self, {
    let res: Result<[u8; KEY_SIZE], _> = buf.read_bytes(KEY_SIZE)?.try_into();
    if res.is_err() {
        return Err(anyhow!("failed to parse the public key"));
    }

    self.pubkey = PublicKey::from_bytes(res.unwrap());

    Ok(())
});

encode_impl!(
    HandshakeResponseData,
    buf,
    self,
    buf.write_bytes(self.pubkey.as_bytes())
);

empty_impl!(
    HandshakeResponseData,
    Self {
        pubkey: PublicKey::from_bytes([0u8; 32])
    }
);

decode_unimpl!(HandshakeResponseData);
