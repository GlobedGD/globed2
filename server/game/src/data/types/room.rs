use crate::data::*;

#[derive(Clone, Copy, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomSettings {
    pub flags: Bits<2>,
    pub reserved: u64, // space for future expansions, should be always 0
}

impl RoomSettings {
    pub fn get_invite_only(&self) -> bool {
        self.flags.get_bit(0)
    }

    pub fn get_public_invites(&self) -> bool {
        self.flags.get_bit(1)
    }

    pub fn get_two_player_mode(&self) -> bool {
        self.flags.get_bit(3)
    }
}

#[derive(Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct RoomInfo {
    pub id: u32,
    pub owner: i32,
    pub token: u32,
    pub settings: RoomSettings,
}
