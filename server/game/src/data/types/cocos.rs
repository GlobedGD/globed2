use anyhow::bail;

use crate::bytebufferext::{decode_impl, encode_impl};

#[derive(Copy, Clone, Default)]
pub struct Color3B {
    pub r: u8,
    pub g: u8,
    pub b: u8,
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

impl TryFrom<String> for Color3B {
    type Error = anyhow::Error;
    fn try_from(value: String) -> Result<Self, Self::Error> {
        if value.len() != 7 {
            bail!("invalid hex string length, expected 7 characters, got {}", value.len());
        }

        if !value.starts_with('#') {
            bail!("invalid hex string, expected '#' at the start");
        }

        let r = u8::from_str_radix(&value[1..3], 16)?;
        let g = u8::from_str_radix(&value[3..5], 16)?;
        let b = u8::from_str_radix(&value[5..7], 16)?;

        Ok(Self { r, g, b })
    }
}

#[derive(Copy, Clone, Default)]
pub struct Color4B {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
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

impl TryFrom<String> for Color4B {
    type Error = anyhow::Error;
    fn try_from(value: String) -> Result<Self, Self::Error> {
        if value.len() != 7 && value.len() != 9 {
            bail!("invalid hex string length, expected 7 or 9 characters, got {}", value.len());
        }

        if !value.starts_with('#') {
            bail!("invalid hex string, expected '#' at the start");
        }

        let r = u8::from_str_radix(&value[1..3], 16)?;
        let g = u8::from_str_radix(&value[3..5], 16)?;
        let b = u8::from_str_radix(&value[5..7], 16)?;
        let a = if value.len() == 9 {
            u8::from_str_radix(&value[7..9], 16)?
        } else {
            0u8
        };

        Ok(Self { r, g, b, a })
    }
}

#[derive(Copy, Clone, Default)]
pub struct Point {
    pub x: f32,
    pub y: f32,
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
