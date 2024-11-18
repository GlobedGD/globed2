use std::fmt::Display;

use esp::DecodeError;

#[derive(Debug)]
pub enum PacketTranslationError {
    UntranslatableSecurity,     // packet cannot be translated due to security reasons
    UntranslatableData,         // packet cannot be translated due to necessary data not being present
    UnsupportedProtocol,        // packet cannot be translated due to the protocol being way too old
    Unimplemented,              // packet cannot be translated due to the translator not being implemented for it
    DecodingError(DecodeError), // packet cannot be translated due to a decoding error
}

impl Display for PacketTranslationError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PacketTranslationError::UntranslatableSecurity => write!(f, "cannot be translated due to security reasons"),
            PacketTranslationError::UntranslatableData => write!(f, "cannot be translated due to the lack of necessary data"),
            PacketTranslationError::UnsupportedProtocol => write!(f, "unsupported protocol version"),
            PacketTranslationError::Unimplemented => write!(f, "unimplemented for this packet"),
            PacketTranslationError::DecodingError(e) => write!(f, "decoding error: {}", e),
        }
    }
}

impl From<DecodeError> for PacketTranslationError {
    fn from(e: DecodeError) -> Self {
        PacketTranslationError::DecodingError(e)
    }
}
