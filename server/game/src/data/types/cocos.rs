use crate::bytebufferext::{decode_impl, empty_impl, encode_impl};

#[derive(Copy, Clone, Default)]
pub struct Color3B {
    r: u8,
    g: u8,
    b: u8,
}

encode_impl!(Color3B, buf, self, {
    buf.write_u8(self.r);
    buf.write_u8(self.g);
    buf.write_u8(self.b);
});

empty_impl!(Color3B, Self { r: 0, g: 0, b: 0 });

decode_impl!(Color3B, buf, self, {
    self.r = buf.read_u8()?;
    self.g = buf.read_u8()?;
    self.b = buf.read_u8()?;
    Ok(())
});

#[derive(Copy, Clone, Default)]
pub struct Color4B {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

encode_impl!(Color4B, buf, self, {
    buf.write_u8(self.r);
    buf.write_u8(self.g);
    buf.write_u8(self.b);
    buf.write_u8(self.a);
});

empty_impl!(
    Color4B,
    Self {
        r: 0,
        g: 0,
        b: 0,
        a: 0
    }
);

decode_impl!(Color4B, buf, self, {
    self.r = buf.read_u8()?;
    self.g = buf.read_u8()?;
    self.b = buf.read_u8()?;
    self.a = buf.read_u8()?;
    Ok(())
});

#[derive(Copy, Clone, Default)]
pub struct Point {
    x: f32,
    y: f32,
}

encode_impl!(Point, buf, self, {
    buf.write_f32(self.x);
    buf.write_f32(self.y);
});

empty_impl!(Point, Self { x: 0f32, y: 0f32 });

decode_impl!(Point, buf, self, {
    self.x = buf.read_f32()?;
    self.y = buf.read_f32()?;
    Ok(())
});
