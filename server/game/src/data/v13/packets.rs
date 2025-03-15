use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 19001, encrypted = true)]
pub struct AdminSendNoticePacket {
    pub notice_type: AdminSendNoticeType,
    pub room_id: u32,
    pub level_id: LevelId,
    pub player: FastString,
    pub message: FastString,
    pub can_reply: bool,
}

#[derive(Packet, Encodable, DynamicSize, Clone)]
#[packet(id = 20100, tcp = false)]
pub struct ServerNoticePacket {
    pub message: FastString,
}
