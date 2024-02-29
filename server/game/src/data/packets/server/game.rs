use crate::data::*;

#[derive(Packet, Encodable)]
#[packet(id = 22000, tcp = true)]
pub struct PlayerProfilesPacket;

#[derive(Packet, Encodable)]
#[packet(id = 22001, tcp = false)]
pub struct LevelDataPacket {
    pub players: Vec<AssociatedPlayerData>,
}

#[derive(Packet, Encodable)]
#[packet(id = 22002, tcp = false)]
pub struct LevelPlayerMetadataPacket {
    pub players: Vec<AssociatedPlayerMetadata>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 22010, encrypted = true, tcp = false)]
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
