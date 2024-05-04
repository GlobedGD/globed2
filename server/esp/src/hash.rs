use simd_adler32::Adler32;

pub fn adler32(data: &[u8]) -> u32 {
    let mut adler = Adler32::new();
    adler.write(data);
    adler.finish()
}

pub const fn adler32_const(data: &'static str) -> u32 {
    const MOD: u32 = 65521;

    let mut a = 1u32;
    let mut b = 0u32;

    let data = data.as_bytes();

    let mut i = 0usize;

    while i < data.len() {
        let c = data[i];
        a = (a + c as u32) % MOD;
        b = (b + a) % MOD;

        i += 1;
    }

    (b << 16) | a
}
