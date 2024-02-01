use crate::data::*;
use globed_shared::ADMIN_KEY_LENGTH;

#[derive(Packet, Decodable)]
#[packet(id = 19000, encrypted = true)]
pub struct AdminAuthPacket {
    pub key: FastString<ADMIN_KEY_LENGTH>,
}

#[derive(Decodable)]
#[repr(u8)]
pub enum AdminSendNoticeType {
    Everyone = 0,
    RoomOrLevel = 1,
    Person = 2,
}

#[derive(Packet, Decodable)]
#[packet(id = 19001)]
pub struct AdminSendNoticePacket {
    pub notice_type: AdminSendNoticeType,
    pub room_id: u32,
    pub level_id: i32,
    pub player: FastString<MAX_NAME_SIZE>,
    pub message: FastString<MAX_NOTICE_SIZE>,
}
