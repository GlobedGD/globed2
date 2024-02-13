use crate::data::*;

/*
* For optimization reasons, these 2 are encoded inline in the packet handlers in server_thread/handlers/game.rs.
*/

#[derive(Packet, Encodable)]
#[packet(id = 22000, tcp = true)]
pub struct PlayerProfilesPacket;

#[derive(Packet, Encodable)]
#[packet(id = 22001, tcp = false)]
pub struct LevelDataPacket;

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 22010, encrypted = true, tcp = true)]
pub struct VoiceBroadcastPacket {
    pub player_id: i32,
    pub data: FastEncodedAudioFrame,
}

#[derive(Clone, Packet, Encodable, StaticSize)]
#[packet(id = 22011, encrypted = true, tcp = false)]
pub struct ChatMessageBroadcastPacket {
    pub player_id: i32,
    pub message: FastString<MAX_MESSAGE_SIZE>,
}
