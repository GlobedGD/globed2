use anyhow::bail;

use crate::bytebufferext::{decode_impl, empty_impl, encode_impl};

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

empty_impl!(Color3B, Self::default());

decode_impl!(Color3B, buf, self, {
    self.r = buf.read_u8()?;
    self.g = buf.read_u8()?;
    self.b = buf.read_u8()?;
    Ok(())
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

empty_impl!(Color4B, Self::default());

decode_impl!(Color4B, buf, self, {
    self.r = buf.read_u8()?;
    self.g = buf.read_u8()?;
    self.b = buf.read_u8()?;
    self.a = buf.read_u8()?;
    Ok(())
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

empty_impl!(Point, Self::default());

decode_impl!(Point, buf, self, {
    self.x = buf.read_f32()?;
    self.y = buf.read_f32()?;
    Ok(())
});
