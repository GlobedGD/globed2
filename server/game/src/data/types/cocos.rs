use crate::bytebufferext::{decode_impl, encode_impl};

#[derive(Copy, Clone)]
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

decode_impl!(Color3B, buf, {
    let r = buf.read_u8()?;
    let g = buf.read_u8()?;
    let b = buf.read_u8()?;
    Ok(Self { r, g, b })
});

#[derive(Copy, Clone)]
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

decode_impl!(Color4B, buf, {
    let r = buf.read_u8()?;
    let g = buf.read_u8()?;
    let b = buf.read_u8()?;
    let a = buf.read_u8()?;
    Ok(Self { r, g, b, a })
});

#[derive(Copy, Clone)]
pub struct Point {
    x: f32,
    y: f32,
}

encode_impl!(Point, buf, self, {
    buf.write_f32(self.x);
    buf.write_f32(self.y);
});

decode_impl!(Point, buf, {
    let x = buf.read_f32()?;
    let y = buf.read_f32()?;
    Ok(Self { x, y })
});
