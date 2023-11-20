use bytebuffer::{ByteBuffer, ByteReader};
use globed_shared::SpecialUser;

use crate::bytebufferext::{
    decode_impl, decode_unimpl, empty_impl, encode_impl, ByteBufferExtWrite, Encodable, FastDecodable,
};

use super::Color3B;

#[derive(Clone)]
pub struct PlayerIconData {
    pub cube: i32,
    pub ship: i32,
    pub ball: i32,
    pub ufo: i32,
    pub wave: i32,
    pub robot: i32,
    pub spider: i32,
    pub swing: i32,
    pub jetpack: i32,
    pub death_effect: i32,
    pub color1: i32,
    pub color2: i32,
}

impl Default for PlayerIconData {
    fn default() -> Self {
        Self {
            cube: 1,
            ship: 1,
            ball: 1,
            ufo: 1,
            wave: 1,
            robot: 1,
            spider: 1,
            swing: 1,
            jetpack: 1,
            death_effect: 1,
            color1: 1,
            color2: 3,
        }
    }
}

encode_impl!(PlayerIconData, buf, self, {
    buf.write_i32(self.cube);
    buf.write_i32(self.ship);
    buf.write_i32(self.ball);
    buf.write_i32(self.ufo);
    buf.write_i32(self.wave);
    buf.write_i32(self.robot);
    buf.write_i32(self.spider);
    buf.write_i32(self.swing);
    buf.write_i32(self.jetpack);
    buf.write_i32(self.death_effect);
    buf.write_i32(self.color1);
    buf.write_i32(self.color2);
});

empty_impl!(PlayerIconData, Self::default());

decode_impl!(PlayerIconData, buf, self, {
    self.cube = buf.read_i32()?;
    self.ship = buf.read_i32()?;
    self.ball = buf.read_i32()?;
    self.ufo = buf.read_i32()?;
    self.wave = buf.read_i32()?;
    self.robot = buf.read_i32()?;
    self.spider = buf.read_i32()?;
    self.swing = buf.read_i32()?;
    self.jetpack = buf.read_i32()?;
    self.death_effect = buf.read_i32()?;
    self.color1 = buf.read_i32()?;
    self.color2 = buf.read_i32()?;
    Ok(())
});

impl PlayerIconData {
    pub fn is_valid(&self) -> bool {
        // TODO icon ids validation and stuff.. or not?
        true
    }
}

/* SpecialUserData */

#[derive(Clone)]
pub struct SpecialUserData {
    pub name_color: Color3B,
}

impl Default for SpecialUserData {
    fn default() -> Self {
        Self {
            name_color: Color3B { r: 255, g: 255, b: 255 },
        }
    }
}

encode_impl!(SpecialUserData, buf, self, {
    buf.write_color3(self.name_color);
});

empty_impl!(SpecialUserData, Self::default());

decode_unimpl!(SpecialUserData);

impl TryFrom<SpecialUser> for SpecialUserData {
    type Error = anyhow::Error;
    fn try_from(value: SpecialUser) -> Result<Self, Self::Error> {
        Ok(Self {
            name_color: value.color.try_into()?,
        })
    }
}

/* PlayerAccountData */

#[derive(Clone, Default)]
pub struct PlayerAccountData {
    pub account_id: i32,
    pub name: String,
    pub icons: PlayerIconData,
    pub special_user_data: Option<SpecialUserData>,
}

encode_impl!(PlayerAccountData, buf, self, {
    buf.write_i32(self.account_id);
    buf.write_string(&self.name);
    buf.write_value(&self.icons);
    buf.write_optional_value(self.special_user_data.as_ref());
});

empty_impl!(PlayerAccountData, Self::default());

decode_unimpl!(PlayerAccountData);

/* PlayerData */

#[derive(Clone, Default)]
pub struct PlayerData {}

encode_impl!(PlayerData, _buf, self, {});

impl FastDecodable for PlayerData {
    fn decode_fast(buf: &mut ByteBuffer) -> anyhow::Result<Self> {
        let mut br = ByteReader::from_bytes(buf.as_bytes());
        Self::decode_fast_from_reader(&mut br)
    }

    fn decode_fast_from_reader(_buf: &mut ByteReader) -> anyhow::Result<Self> {
        Ok(Self {})
    }
}

impl PlayerData {
    const fn encoded_size() -> usize {
        0
    }
}

/* AssociatedPlayerData */

#[derive(Clone, Default)]
pub struct AssociatedPlayerData {
    pub account_id: i32,
    pub data: PlayerData,
}

impl Encodable for AssociatedPlayerData {
    fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_i32(self.account_id);
        buf.write_value(&self.data);
    }
}

impl AssociatedPlayerData {
    pub const fn encoded_size() -> usize {
        std::mem::size_of::<i32>() + PlayerData::encoded_size()
    }
}
