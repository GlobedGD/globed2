use std::borrow::Cow;

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
    pub message: Cow<'a, str>,
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
    pub message: Cow<'a, str>,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29004, tcp, encrypted)]
pub struct AdminAuthFailedPacket;

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29005, tcp, encrypted)]
pub struct AdminPunishmentHistoryPacket {
    pub entries: Vec<UserPunishment>,
    pub mod_name_data: Vec<(i32, String)>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29006, tcp, encrypted)]
pub struct AdminSuccessfulUpdatePacket {
    pub user_entry: UserEntry,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 29007, tcp, encrypted)]
pub struct AdminReceivedNoticeReplyPacket {
    pub reply_id: u32,
    pub user_id: i32,
    pub user_name: FastString,
    pub admin_msg: FastString,
    pub user_reply: FastString,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29008, tcp, encrypted)]
pub struct AdminNoticeRecipientCountPacket {
    pub count: u32,
}
