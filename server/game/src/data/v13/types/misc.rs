use crate::data::*;

#[derive(Encodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct GlobedLevel {
    pub level_id: LevelId,
    pub player_count: u16,
}

#[derive(Encodable, Decodable, StaticSize, DynamicSize)]
#[dynamic_size(as_static = true)]
pub struct ErrorMessage {
    pub hash: u32,
}

impl ErrorMessage {
    pub const fn new(string: &'static str) -> Self {
        Self {
            hash: esp::hash::adler32_const(string),
        }
    }

    pub const fn new_with_hash(hash: u32) -> Self {
        Self { hash }
    }
}

#[derive(Encodable, Decodable, DynamicSize)]
// Can be either a builtin error message hash or a custom message string
pub struct CustomErrorMessage {
    pub message: Either<ErrorMessage, String>,
}

impl CustomErrorMessage {
    pub const fn builtin(string: &'static str) -> Self {
        Self {
            message: Either::new_first(ErrorMessage::new(string)),
        }
    }

    pub const fn builtin_with_hash(hash: u32) -> Self {
        Self {
            message: Either::new_first(ErrorMessage::new_with_hash(hash)),
        }
    }

    pub const fn custom(string: String) -> Self {
        Self {
            message: Either::new_second(string),
        }
    }
}
