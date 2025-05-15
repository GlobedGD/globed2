use super::*;
use crate::client::PacketTranslationResult;
use crate::data::v_current;
use crate::data::v13;

impl Translatable for AdminAuthPacket {}
impl Translatable for AdminSendNoticePacket {
    fn translate_from(data: &mut esp::ByteReader<'_>, protocol: u16) -> PacketTranslationResult<Self> {
        match protocol {
            13 => {
                let old_packet = v13::AdminSendNoticePacket::decode_from_reader(data)?;

                Ok(v_current::AdminSendNoticePacket {
                    notice_type: old_packet.notice_type,
                    room_id: old_packet.room_id,
                    level_id: old_packet.level_id,
                    player: old_packet.player,
                    message: old_packet.message,
                    can_reply: false,
                    just_estimate: false,
                })
            }

            _ => Err(PacketTranslationError::UnsupportedProtocol),
        }
    }
}

impl Translatable for AdminDisconnectPacket {}
impl Translatable for AdminGetUserStatePacket {}
impl Translatable for AdminSendFeaturedLevelPacket {}
impl Translatable for AdminUpdateUsernamePacket {}
impl Translatable for AdminSetNameColorPacket {}
impl Translatable for AdminSetUserRolesPacket {}
impl Translatable for AdminPunishUserPacket {}
impl Translatable for AdminRemovePunishmentPacket {}
impl Translatable for AdminWhitelistPacket {}
impl Translatable for AdminSetAdminPasswordPacket {}
impl Translatable for AdminEditPunishmentPacket {}
impl Translatable for AdminGetPunishmentHistoryPacket {}
