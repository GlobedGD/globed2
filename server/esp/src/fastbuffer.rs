/// Buffer for encoding that does zero heap allocation but also has limited functionality.
/// It will panic on writes if there isn't enough space.
/// On average, is at least 4-5x faster than a regular `ByteBuffer`.
pub struct FastByteBuffer<'a> {
    pos: usize,
    len: usize,
    data: &'a mut [u8],
}

#[allow(clippy::inline_always)]
impl<'a> FastByteBuffer<'a> {
    /// Create a new `FastByteBuffer` given this mutable slice
    pub fn new(src: &'a mut [u8]) -> Self {
        Self { pos: 0, len: 0, data: src }
    }

    pub fn new_with_length(src: &'a mut [u8], len: usize) -> Self {
        Self { pos: 0, len, data: src }
    }

    #[inline(always)]
    pub fn write_u8(&mut self, val: u8) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_u16(&mut self, val: u16) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_u32(&mut self, val: u32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_u64(&mut self, val: u64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_i8(&mut self, val: i8) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_i16(&mut self, val: i16) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_i32(&mut self, val: i32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_i64(&mut self, val: i64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_f32(&mut self, val: f32) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_f64(&mut self, val: f64) {
        self.internal_write(&val.to_be_bytes());
    }

    #[inline(always)]
    pub fn write_bytes(&mut self, data: &[u8]) {
        self.internal_write(data);
    }

    #[inline]
    pub fn as_bytes(&'a self) -> &'a [u8] {
        &self.data[..self.len]
    }

    #[inline]
    pub const fn len(&self) -> usize {
        self.len
    }

    #[inline]
    pub const fn get_pos(&self) -> usize {
        self.pos
    }

    #[inline]
    pub fn set_pos(&mut self, pos: usize) {
        self.pos = pos;
    }

    #[inline]
    pub const fn is_empty(&self) -> bool {
        self.len == 0
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.data.len()
    }

    /// Write the given byte slice. Panics if there is not enough capacity left to write the data.
    #[inline]
    fn internal_write(&mut self, data: &[u8]) {
        debug_assert!(
            self.pos + data.len() <= self.capacity(),
            "not enough space to write data into FastByteBuffer, capacity: {}, pos: {}, attempted write size: {}",
            self.capacity(),
            self.pos,
            data.len()
        );

        self.data[self.pos..self.pos + data.len()].copy_from_slice(data);
        self.pos += data.len();
        self.len = self.len.max(self.pos);
    }

    #[inline]
    pub fn to_vec(&self) -> Vec<u8> {
        self.as_bytes().to_vec()
    }
}
