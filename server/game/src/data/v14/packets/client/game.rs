use crate::data::*;

#[derive(Packet, Decodable)]
#[packet(id = 12000)]
pub struct RequestPlayerProfilesPacket {
    pub requested: i32, // 0 to get all ppl on the level
}

#[derive(Packet, Decodable)]
#[packet(id = 12001)]
pub struct LevelJoinPacket {
    pub level_id: LevelId,
    pub unlisted: bool,
    pub level_hash: Option<ByteArray<32>>,
}

#[derive(Packet, Decodable)]
#[packet(id = 12002)]
pub struct LevelLeavePacket;

#[derive(Packet, Decodable)]
#[packet(id = 12003)]
pub struct PlayerDataPacket {
    pub data: PlayerData,
    pub meta: Option<PlayerMetadata>,
    pub counter_changes: Vec1L<GlobedCounterChange>,
}

#[derive(Packet, Decodable)]
#[packet(id = 12010, encrypted = true)]
pub struct VoicePacket {
    pub data: FastEncodedAudioFrame,
}

#[derive(Packet, Decodable)]
#[packet(id = 12011, encrypted = true)]
pub struct ChatMessagePacket {
    pub message: InlineString<MAX_MESSAGE_SIZE>,
}
