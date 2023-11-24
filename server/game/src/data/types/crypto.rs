use std::io::Read;

use crypto_box::{PublicKey, KEY_SIZE};

use crate::data::bytebufferext::*;

#[derive(Clone)]
pub struct CryptoPublicKey {
    pub pubkey: PublicKey,
}

encode_impl!(CryptoPublicKey, buf, self, buf.write_bytes(self.pubkey.as_bytes()));

decode_impl!(CryptoPublicKey, buf, {
    let mut key = [0u8; KEY_SIZE];
    buf.read_exact(&mut key)?;

    Ok(Self {
        pubkey: PublicKey::from_bytes(key),
    })
});

size_calc_impl!(CryptoPublicKey, KEY_SIZE);
