use simd_adler32::Adler32;

pub fn adler32(data: &[u8]) -> u32 {
    let mut adler = Adler32::new();
    adler.write(data);
    adler.finish()
}
