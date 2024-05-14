use crate::data::*;
use globed_shared::{UserEntry, ADMIN_KEY_LENGTH};

#[derive(Packet, Decodable)]
#[packet(id = 19000, encrypted = true)]
pub struct AdminAuthPacket {
    pub key: FastString,
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
    pub level_id: LevelId,
    pub player: FastString,
    pub message: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19002)]
pub struct AdminDisconnectPacket {
    pub player: FastString,
    pub message: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19003)]
pub struct AdminGetUserStatePacket {
    pub player: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19004)]
pub struct AdminUpdateUserPacket {
    pub user_entry: UserEntry,
}
