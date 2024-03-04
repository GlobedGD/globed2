use globed_shared::UserEntry;

use crate::data::*;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29000, tcp = true)]
pub struct AdminAuthSuccessPacket {
    pub role: i32,
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
#[packet(id = 29003, tcp = true)]
pub struct AdminSuccessMessagePacket<'a> {
    pub message: &'a str,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29004, tcp = true)]
pub struct AdminAuthFailedPacket;
