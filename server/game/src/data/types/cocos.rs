use std::{fmt::Display, num::ParseIntError, str::FromStr};

use crate::data::bytebufferext::*;

pub enum ColorParseError {
    InvalidLength,
    InvalidFormat,
    ParseError,
}

impl Display for ColorParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ColorParseError::InvalidFormat => f.write_str("invalid hex string, expected '#' at the start"),
            ColorParseError::InvalidLength => {
                f.write_str("invalid length of the hex string, should start with '#' and have 6 characters for Color3B or 6/8 characters for Color4B")
            },
            ColorParseError::ParseError => f.write_str("invalid hex bytes encountered in the string")
        }
    }
}

impl From<ParseIntError> for ColorParseError {
    fn from(_: ParseIntError) -> Self {
        ColorParseError::ParseError
    }
}

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

size_calc_impl!(Color3B, size_of_types!(u8, u8, u8));

impl FromStr for Color3B {
    type Err = ColorParseError;

    fn from_str(value: &str) -> Result<Self, Self::Err> {
        if value.len() != 7 {
            return Err(ColorParseError::InvalidLength);
        }

        if !value.starts_with('#') {
            return Err(ColorParseError::InvalidFormat);
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

size_calc_impl!(Color4B, size_of_types!(u8, u8, u8, u8));

impl FromStr for Color4B {
    type Err = ColorParseError;

    fn from_str(value: &str) -> Result<Self, Self::Err> {
        if value.len() != 7 && value.len() != 9 {
            return Err(ColorParseError::InvalidLength);
        }

        if !value.starts_with('#') {
            return Err(ColorParseError::InvalidFormat);
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

size_calc_impl!(Point, size_of_types!(f32, f32));
