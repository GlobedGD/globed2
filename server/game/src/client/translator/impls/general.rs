use super::*;
use crate::client::PacketTranslationResult;
use crate::client::PartialTranslatableEncodable;
use crate::client::PartialTranslatableTo;
use crate::data::v13;
use esp::ByteBufferExtWrite;

impl Translatable for SyncIconsPacket {}
impl Translatable for RequestGlobalPlayerListPacket {}
impl Translatable for RequestLevelListPacket {}
impl Translatable for RequestPlayerCountPacket {}
impl Translatable for UpdatePlayerStatusPacket {}
impl Translatable for LinkCodeRequestPacket {}
impl Translatable for NoticeReplyPacket {}
impl Translatable for RequestMotdPacket {}

// Partial impls

impl PartialTranslatableTo<v13::ServerNoticePacket> for ServerNoticePacket {
    fn translate_to(self) -> PacketTranslationResult<v13::ServerNoticePacket> {
        Ok(v13::ServerNoticePacket { message: self.message })
    }
}

impl PartialTranslatableEncodable for ServerNoticePacket {
    fn translate_and_encode(self, protocol: u16, buf: &mut esp::ByteBuffer) -> PacketTranslationResult<()> {
        match protocol {
            13 => {
                let translated = <Self as PartialTranslatableTo<v13::ServerNoticePacket>>::translate_to(self)?;
                buf.write_value(&translated);
                Ok(())
            }

            _ => Err(PacketTranslationError::UnsupportedProtocol),
        }
    }
}
