use globed_shared::{UserEntry, UserPunishment};

use crate::{data::*, managers::ComputedRole};

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29000, tcp = true)]
pub struct AdminAuthSuccessPacket {
    pub role: ComputedRole,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29001, tcp = true, encrypted = true)]
pub struct AdminErrorPacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29002, tcp = true, encrypted = true)]
pub struct AdminUserDataPacket {
    pub entry: UserEntry,
    pub account_data: Option<PlayerRoomPreviewAccountData>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29003, tcp, encrypted)]
pub struct AdminSuccessMessagePacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29004, tcp, encrypted)]
pub struct AdminAuthFailedPacket;

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29005, tcp, encrypted)]
pub struct AdminPunishmentHistoryPacket {
    pub entries: Vec<UserPunishment>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29006, tcp, encrypted)]
pub struct AdminSuccessfulPunishmentPacket {
    pub punishment: UserPunishment,
}
