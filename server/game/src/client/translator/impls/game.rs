use super::*;
use crate::{client::PacketTranslationResult, data::v_current, data::v13};

impl Translatable for LevelJoinPacket {
    fn translate_from(data: &mut esp::ByteReader<'_>, protocol: u16) -> PacketTranslationResult<Self> {
        match protocol {
            13 => {
                let old_packet = v13::LevelJoinPacket::decode_from_reader(data)?;

                Ok(v_current::LevelJoinPacket {
                    level_id: old_packet.level_id,
                    unlisted: old_packet.unlisted,
                    level_hash: None,
                })
            }

            _ => Err(PacketTranslationError::UnsupportedProtocol),
        }
    }
}
impl Translatable for LevelLeavePacket {}
impl Translatable for PlayerDataPacket {}
impl Translatable for RequestPlayerProfilesPacket {}
impl Translatable for VoicePacket {}
impl Translatable for ChatMessagePacket {}
