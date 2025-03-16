use std::io::Read;

use esp::*;
use globed_shared::crypto_box::{KEY_SIZE, PublicKey};

pub struct CryptoPublicKey(pub PublicKey);

impl From<PublicKey> for CryptoPublicKey {
    fn from(value: PublicKey) -> Self {
        Self(value)
    }
}

encode_impl!(CryptoPublicKey, buf, self, {
    buf.write_bytes(self.0.as_bytes());
});

decode_impl!(CryptoPublicKey, buf, {
    let mut key = [0u8; KEY_SIZE];
    buf.read_exact(&mut key)?;

    Ok(Self(PublicKey::from_bytes(key)))
});

static_size_calc_impl!(CryptoPublicKey, KEY_SIZE);
dynamic_size_calc_impl!(CryptoPublicKey, self, KEY_SIZE);
