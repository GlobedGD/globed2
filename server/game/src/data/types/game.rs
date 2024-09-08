use crate::data::*;

/* PlayerIconType */

#[derive(Default, Debug, Copy, Clone, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
#[repr(u8)]
pub enum PlayerIconType {
    #[default]
    Unknown = 0,
    Cube = 1,
    Ship = 2,
    Ball = 3,
    Ufo = 4,
    Wave = 5,
    Robot = 6,
    Spider = 7,
    Swing = 8,
    Jetpack = 9,
}

/* SpiderTeleportData (spider teleport data) */
#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct SpiderTeleportData {
    pub from: Point,
    pub to: Point,
}

/* SpecificIconData (specific player data) */
// 16 bytes best-case, 32 bytes worst-case (when on the same frame as spider TP).

#[derive(Clone, Debug, Default, Encodable, Decodable, StaticSize, DynamicSize)]
pub struct SpecificIconData {
    pub position: Point,
    pub rotation: FiniteF32,
    pub icon_type: PlayerIconType,
    pub flags: Bits<2>, // bit-field with various flags, see the client-side structure for more info
    pub spider_teleport_data: Option<SpiderTeleportData>,
}

/* PlayerMetadata (player things that are sent less often) */

#[derive(Clone, Default, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct PlayerMetadata {
    pub local_best: u32, // percentage or milliseconds in platformer
    pub attempts: i32,
}

#[derive(Clone, Copy, Debug, Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static)]
#[repr(u8)]
pub enum GlobedCounterChangeType {
    Set = 0,
    Add = 1,
    Multiply = 2,
    Divide = 3,
}

#[derive(Clone, Copy)]
union CCValue {
    pub int_val: i32,
    pub flt_val: FiniteF32,
}

static_size_calc_impl!(CCValue, size_of_types!(i32));
dynamic_size_as_static_impl!(CCValue);

/* GlobedCounterChange */
#[derive(Clone, StaticSize, DynamicSize)]
#[dynamic_size(as_static)]
pub struct GlobedCounterChange {
    pub item_id: u16,
    pub r#type: GlobedCounterChangeType,
    value: CCValue, // its a union but whatever
}

decode_impl!(GlobedCounterChange, buf, {
    let item_id = buf.read_value()?;
    let r#type = buf.read_value()?;

    let value = match r#type {
        GlobedCounterChangeType::Add | GlobedCounterChangeType::Set => CCValue { int_val: buf.read_i32()? },
        GlobedCounterChangeType::Multiply | GlobedCounterChangeType::Divide => CCValue { flt_val: buf.read_value()? },
    };

    Ok(Self { item_id, r#type, value })
});

impl GlobedCounterChange {
    pub fn apply_to(&self, number_ref: &mut i32) {
        let number = *number_ref;
        // safety: set/add are always ints, mul/div are always float
        let value = unsafe {
            match self.r#type {
                GlobedCounterChangeType::Add => number + self.value.int_val,
                GlobedCounterChangeType::Set => self.value.int_val,
                GlobedCounterChangeType::Multiply => ((number as f32) * self.value.flt_val.0) as i32,
                GlobedCounterChangeType::Divide => {
                    if self.value.flt_val.0 == 0.0f32 {
                        number
                    } else {
                        ((number as f32) / self.value.flt_val.0) as i32
                    }
                }
            }
        };

        *number_ref = value;
    }
}

/* PlayerData (data in a level) */
// 45 bytes best-case, 77 bytes worst-case (with 2 spider teleports).

#[derive(Clone, Debug, Default, Encodable, Decodable, DynamicSize)]
pub struct PlayerData {
    pub timestamp: FiniteF32,

    pub player1: SpecificIconData,
    pub player2: SpecificIconData,

    pub last_death_timestamp: FiniteF32,

    pub current_percentage: FiniteF32,

    pub flags: Bits<1>, // also a bit-field
}
