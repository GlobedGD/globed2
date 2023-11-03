use blake2::{Blake2b, Blake2bCore, Blake2s256, Digest};
use digest::consts::U32;
use totp_rs::{Algorithm, Secret, TOTP};

fn main() {
    let mut hasher = Blake2b::<U32>::new();
    hasher.update("My epic key");

    let output = hasher.finalize();

    let secret = Secret::Raw(output.to_vec()).to_bytes().unwrap();

    let totp = TOTP::new(Algorithm::SHA256, 6, 1, 30, secret).unwrap();
    let rn = totp.generate_current().unwrap();
    println!("totp: {}", rn);
}
