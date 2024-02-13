use crate::data::*;

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 29000, tcp = true)]
pub struct AdminAuthSuccessPacket;
