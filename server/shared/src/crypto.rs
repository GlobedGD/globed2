use crypto_box::{
    ChaChaBox, PublicKey, SecretKey,
    aead::{Aead, AeadCore, AeadInPlace, OsRng, generic_array::GenericArray},
};
use crypto_secretbox::{
    KeyInit, XChaCha20Poly1305,
    consts::{U24, U32},
};

/// Simpler interface for encryption/decryption
pub enum CryptoBox {
    Shared(ChaChaBox),
    Secret(XChaCha20Poly1305),
}

#[derive(Debug)]
pub struct CryptoBoxError;

type Result<T> = std::result::Result<T, CryptoBoxError>;

// this is funny
macro_rules! run_thing {
    ($self:expr, $box:ident, $code:expr) => {
        match $self {
            CryptoBox::Shared($box) => $code,
            CryptoBox::Secret($box) => $code,
        }
    };
}

// Format - nonce -> mac -> data (prefix mac)

impl CryptoBox {
    pub const fn nonce_size() -> usize {
        24
    }

    pub const fn mac_size() -> usize {
        16
    }

    pub const fn prefix_len() -> usize {
        Self::nonce_size() + Self::mac_size()
    }

    pub const fn calculate_message_len(len: usize) -> usize {
        len + Self::prefix_len()
    }

    pub fn new_shared(public_key: &PublicKey, secret_key: &SecretKey) -> Self {
        Self::Shared(ChaChaBox::new(public_key, secret_key))
    }

    pub fn new_secret(key: &[u8]) -> Self {
        assert!(key.len() == 32, "Secret key must be 32 bytes in size");

        let mut gkey = GenericArray::<u8, U32>::default();
        gkey.clone_from_slice(key);

        Self::Secret(XChaCha20Poly1305::new(&gkey))
    }

    pub fn encrypt(&self, data: &[u8]) -> Result<Vec<u8>> {
        let nonce = ChaChaBox::generate_nonce(&mut OsRng);
        self.encrypt_with_nonce(data, nonce)
    }

    pub fn encrypt_with_nonce(&self, data: &[u8], nonce: GenericArray<u8, U24>) -> Result<Vec<u8>> {
        run_thing!(self, b, b.encrypt(&nonce, data).map_err(|_| CryptoBoxError))
    }

    pub fn decrypt(&self, data: &[u8]) -> Result<Vec<u8>> {
        let nonce_start = 0;
        let mac_start = nonce_start + Self::nonce_size();
        let ciphertext_start = mac_start + Self::mac_size();

        let mut nonce = [0u8; Self::nonce_size()];
        nonce.clone_from_slice(&data[0..Self::nonce_size()]);
        let nonce = nonce.into();

        let mut mac = [0u8; Self::mac_size()];
        mac.clone_from_slice(&data[mac_start..ciphertext_start]);
        let mac = mac.into();

        // put the plaintext data into the vector
        let mut out = Vec::with_capacity(data.len() - Self::prefix_len());
        out.extend_from_slice(&data[ciphertext_start..]);

        // decrypt in-place
        match run_thing!(self, b, b.decrypt_in_place_detached(&nonce, b"", &mut out[ciphertext_start..], &mac)) {
            Ok(()) => Ok(out),
            Err(_) => Err(CryptoBoxError),
        }
    }

    /// Decrypts the data in-place, without additional allocations. Returns the amount of size in the plaintext.
    /// Everything past that size should not be read.
    pub fn decrypt_in_place(&self, data: &mut [u8]) -> Result<usize> {
        let nonce_start = 0;
        let mac_start = nonce_start + Self::nonce_size();
        let ciphertext_start = mac_start + Self::mac_size();

        let mut nonce = [0u8; Self::nonce_size()];
        nonce.clone_from_slice(&data[0..Self::nonce_size()]);
        let nonce = nonce.into();

        let mut mac = [0u8; Self::mac_size()];
        mac.clone_from_slice(&data[mac_start..ciphertext_start]);
        let mac = mac.into();

        // memmove everything after prefix to the start
        data.copy_within(ciphertext_start..data.len(), 0);

        let plaintext_len = data.len() - ciphertext_start;

        // decrypt
        match run_thing!(self, b, b.decrypt_in_place_detached(&nonce, b"", &mut data[..plaintext_len], &mac)) {
            Ok(()) => Ok(plaintext_len),
            Err(_) => Err(CryptoBoxError),
        }
    }
}
