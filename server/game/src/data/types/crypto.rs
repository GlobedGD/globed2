use crypto_box::{PublicKey, KEY_SIZE};

use crate::bytebufferext::*;

#[derive(Clone)]
pub struct CryptoPublicKey {
    pub pubkey: PublicKey,
}

encode_impl!(CryptoPublicKey, buf, self, buf.write_bytes(self.pubkey.as_bytes()));

empty_impl!(
    CryptoPublicKey,
    Self {
        pubkey: PublicKey::from_bytes([0u8; 32])
    }
);

decode_impl!(CryptoPublicKey, buf, self, {
    let mut key = [0u8; KEY_SIZE];
    buf.read_bytes_into(&mut key)?;

    self.pubkey = PublicKey::from_bytes(key);

    Ok(())
});
