use globed_shared::UserEntry;

use crate::data::*;

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
#[packet(id = 19017, encrypted = true)]
pub struct AdminEditPunishmentPacket {
    pub account_id: i32,
    pub is_ban: bool,
    pub reason: FastString,
    pub expires_at: u64,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29002, tcp = true, encrypted = true)]
pub struct AdminUserDataPacket {
    pub entry: UserEntry,
    pub account_data: Option<PlayerRoomPreviewAccountData>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29006, tcp, encrypted)]
pub struct AdminSuccessfulUpdatePacket {
    pub user_entry: UserEntry,
}
