use crate::data::*;

/*
* For optimization reasons, most of those packets are encoded inline, and their structure is not present here.
*/

#[derive(Packet, Encodable)]
#[packet(id = 21000, tcp = true)]
pub struct GlobalPlayerListPacket; // definition intentionally missing

#[derive(Packet, Encodable)]
#[packet(id = 21001, tcp = true)]
pub struct LevelListPacket; // definition intentionally missing

#[derive(Packet, Encodable)]
#[packet(id = 21002)]
pub struct LevelPlayerCountPacket; // definition intentionally missing
