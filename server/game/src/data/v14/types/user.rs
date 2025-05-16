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

#[derive(Clone, Encodable, Decodable)]
pub struct UserPrivacyFlags {
    flags: Bits<1>,
}

impl UserPrivacyFlags {
    pub fn get_hide_from_lists(&self) -> bool {
        self.flags.get_bit(0)
    }

    pub fn set_hide_from_lists(&mut self, s: bool) {
        self.flags.assign_bit(0, s);
    }

    pub fn get_no_invites(&self) -> bool {
        self.flags.get_bit(1)
    }

    pub fn set_no_invites(&mut self, s: bool) {
        self.flags.assign_bit(1, s);
    }

    pub fn get_hide_in_game(&self) -> bool {
        self.flags.get_bit(2)
    }

    pub fn set_hide_in_game(&mut self, s: bool) {
        self.flags.assign_bit(2, s);
    }

    pub fn get_hide_roles(&self) -> bool {
        self.flags.get_bit(3)
    }

    pub fn set_hide_roles(&mut self, s: bool) {
        self.flags.assign_bit(3, s);
    }

    // TODO: really shouldn't be in this struct
    pub fn get_tcp_audio(&self) -> bool {
        self.flags.get_bit(4)
    }
}

impl Default for UserPrivacyFlags {
    fn default() -> Self {
        let mut returned = Self { flags: Bits::new() };

        returned.set_hide_from_lists(false);
        returned.set_no_invites(false);
        returned.set_hide_in_game(false);
        returned.set_hide_roles(false);

        returned
    }
}
