use anyhow::anyhow;
use crypto_box::{PublicKey, KEY_SIZE};

use crate::bytebufferext::*;

#[derive(Clone)]
pub struct CryptoPublicKey {
    pub pubkey: PublicKey,
}

encode_impl!(
    CryptoPublicKey,
    buf,
    self,
    buf.write_bytes(self.pubkey.as_bytes())
);

empty_impl!(
    CryptoPublicKey,
    Self {
        pubkey: PublicKey::from_bytes([0u8; 32])
    }
);

decode_impl!(CryptoPublicKey, buf, self, {
    let res: Result<[u8; KEY_SIZE], _> = buf.read_bytes(KEY_SIZE)?.try_into();
    if res.is_err() {
        return Err(anyhow!("failed to parse the public key"));
    }

    self.pubkey = PublicKey::from_bytes(res.unwrap());

    Ok(())
});
