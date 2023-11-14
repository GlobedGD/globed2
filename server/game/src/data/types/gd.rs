use crate::bytebufferext::{decode_impl, decode_unimpl, empty_impl, encode_impl, ByteBufferExtWrite};

// TODO jetpack i think??
#[derive(Clone, Default)]
pub struct PlayerIconData {
    pub cube: i32,
    pub ship: i32,
    pub ball: i32,
    pub ufo: i32,
    pub wave: i32,
    pub robot: i32,
    pub spider: i32,
    pub swing: i32,
    pub death_effect: i32,
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
    buf.write_i32(self.death_effect);
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
    self.death_effect = buf.read_i32()?;
    Ok(())
});

impl PlayerIconData {
    pub fn is_valid(&self) -> bool {
        // TODO icon ids validation and stuff..
        true
    }
}

#[derive(Clone, Default)]
pub struct PlayerAccountData {
    pub account_id: i32,
    pub name: String,
    pub icons: PlayerIconData,
}

encode_impl!(PlayerAccountData, buf, self, {
    buf.write_i32(self.account_id);
    buf.write_string(&self.name);
    buf.write_value(&self.icons);
});

empty_impl!(PlayerAccountData, Self::default());

decode_unimpl!(PlayerAccountData);
