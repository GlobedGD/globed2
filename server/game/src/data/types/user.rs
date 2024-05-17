use std::str::FromStr;

use crate::data::*;

/// Structure that can represent either a single RGB color, or a combination of colors
#[derive(Clone, Encodable, Decodable, StaticSize, DynamicSize)]
pub struct RichColor {
    pub color: Either<Color3B, FastVec<Color3B, 8>>,
}

impl RichColor {
    pub const fn new_with_one(color: Color3B) -> Self {
        Self {
            color: Either::new_first(color),
        }
    }

    pub fn new_with_multiple(colors: Vec<Color3B>) -> Self {
        Self {
            color: Either::new_second(colors.try_into().expect("failed to convert Vec into FastVec for RichColor")),
        }
    }
}

impl FromStr for RichColor {
    type Err = ColorParseError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        // if it's a single color, try to just do that
        if !s.contains('>') {
            let col = s.parse::<Color3B>()?;
            return Ok(Self::new_with_one(col));
        }

        let s = s.strip_prefix('#').unwrap_or(s);

        let mut pieces = s.split('>').map(|x| x.trim().parse::<Color3B>());

        match pieces.find(|x| x.is_err()) {
            Some(err) => Err(err.unwrap_err()),
            None => Ok(Self::new_with_multiple(pieces.map(|x| x.unwrap()).collect())),
        }
    }
}
