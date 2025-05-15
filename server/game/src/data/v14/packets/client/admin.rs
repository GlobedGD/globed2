use crate::data::*;

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
#[packet(id = 19001, encrypted = true)]
pub struct AdminSendNoticePacket {
    pub notice_type: AdminSendNoticeType,
    pub room_id: u32,
    pub level_id: LevelId,
    pub player: FastString,
    pub message: FastString,
    pub can_reply: bool,
    pub just_estimate: bool,
}

#[derive(Packet, Decodable)]
#[packet(id = 19002, encrypted = true)]
pub struct AdminDisconnectPacket {
    pub player: FastString,
    pub message: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19003, encrypted = true)]
pub struct AdminGetUserStatePacket {
    pub player: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19005, encrypted = true)]
pub struct AdminSendFeaturedLevelPacket {
    pub level_name: FastString,
    pub level_id: i32,
    pub level_author: FastString,
    pub difficulty: i32,
    pub rate_tier: i32,
    pub notes: Option<FastString>,
}

// Update user packets

#[derive(Packet, Decodable)]
#[packet(id = 19010, encrypted = true)]
pub struct AdminUpdateUsernamePacket {
    pub account_id: i32,
    pub username: InlineString<MAX_NAME_SIZE>,
}

#[derive(Packet, Decodable)]
#[packet(id = 19011, encrypted = true)]
pub struct AdminSetNameColorPacket {
    pub account_id: i32,
    pub color: Color3B,
}

#[derive(Packet, Decodable)]
#[packet(id = 19012, encrypted = true)]
pub struct AdminSetUserRolesPacket {
    pub account_id: i32,
    pub roles: Vec<String>,
}

#[derive(Packet, Decodable)]
#[packet(id = 19013, encrypted = true)]
pub struct AdminPunishUserPacket {
    pub account_id: i32,
    pub is_ban: bool,
    pub reason: FastString,
    pub expires_at: u64,
}

#[derive(Packet, Decodable)]
#[packet(id = 19014, encrypted = true)]
pub struct AdminRemovePunishmentPacket {
    pub account_id: i32,
    pub is_ban: bool,
}

#[derive(Packet, Decodable)]
#[packet(id = 19015, encrypted = true)]
pub struct AdminWhitelistPacket {
    pub account_id: i32,
    pub state: bool,
}

#[derive(Packet, Decodable)]
#[packet(id = 19016, encrypted = true)]
pub struct AdminSetAdminPasswordPacket {
    pub account_id: i32,
    pub new_password: FastString,
}

#[derive(Packet, Decodable)]
#[packet(id = 19017, encrypted = true)]
pub struct AdminEditPunishmentPacket {
    pub account_id: i32,
    pub is_ban: bool,
    pub reason: FastString,
    pub expires_at: u64,
}

#[derive(Packet, Decodable)]
#[packet(id = 19018, encrypted = true)]
pub struct AdminGetPunishmentHistoryPacket {
    pub account_id: i32,
}
