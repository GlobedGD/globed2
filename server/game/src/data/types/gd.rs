use globed_shared::SpecialUser;

use crate::bytebufferext::{decode_impl, decode_unimpl, encode_impl, ByteBufferExtWrite};

use super::Color3B;

#[derive(Clone)]
pub struct PlayerIconData {
    pub cube: i16,
    pub ship: i16,
    pub ball: i16,
    pub ufo: i16,
    pub wave: i16,
    pub robot: i16,
    pub spider: i16,
    pub swing: i16,
    pub jetpack: i16,
    pub death_effect: i16,
    pub color1: i16,
    pub color2: i16,
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
    buf.write_i16(self.cube);
    buf.write_i16(self.ship);
    buf.write_i16(self.ball);
    buf.write_i16(self.ufo);
    buf.write_i16(self.wave);
    buf.write_i16(self.robot);
    buf.write_i16(self.spider);
    buf.write_i16(self.swing);
    buf.write_i16(self.jetpack);
    buf.write_i16(self.death_effect);
    buf.write_i16(self.color1);
    buf.write_i16(self.color2);
});

decode_impl!(PlayerIconData, buf, {
    let cube = buf.read_i16()?;
    let ship = buf.read_i16()?;
    let ball = buf.read_i16()?;
    let ufo = buf.read_i16()?;
    let wave = buf.read_i16()?;
    let robot = buf.read_i16()?;
    let spider = buf.read_i16()?;
    let swing = buf.read_i16()?;
    let jetpack = buf.read_i16()?;
    let death_effect = buf.read_i16()?;
    let color1 = buf.read_i16()?;
    let color2 = buf.read_i16()?;
    Ok(Self {
        cube,
        ship,
        ball,
        ufo,
        wave,
        robot,
        spider,
        swing,
        jetpack,
        death_effect,
        color1,
        color2,
    })
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

decode_unimpl!(PlayerAccountData);

/* PlayerData */

#[derive(Clone, Default)]
pub struct PlayerData {}

encode_impl!(PlayerData, _buf, self, {});

decode_impl!(PlayerData, _buf, Ok(Self {}));

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

encode_impl!(AssociatedPlayerData, buf, self, {
    buf.write_i32(self.account_id);
    buf.write_value(&self.data);
});

impl AssociatedPlayerData {
    pub const fn encoded_size() -> usize {
        std::mem::size_of::<i32>() + PlayerData::encoded_size()
    }
}
