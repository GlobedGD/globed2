use crate::data::*;

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11000, encrypted = false)]
pub struct SyncIconsPacket {
    pub icons: PlayerIconData,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11001, encrypted = false)]
pub struct RequestProfilesPacket {
    pub ids: [i32; MAX_PROFILES_REQUESTED],
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11002, encrypted = false)]
pub struct LevelJoinPacket {
    pub level_id: i32,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11003, encrypted = false)]
pub struct LevelLeavePacket {}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11004, encrypted = false)]
pub struct PlayerDataPacket {
    pub data: PlayerData,
}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11005, encrypted = false)]
pub struct RequestPlayerListPacket {}

#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11006, encrypted = false)]
pub struct SyncPlayerMetadataPacket {
    pub data: PlayerMetadata,
}

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 11010, encrypted = true)]
pub struct VoicePacket {
    pub data: FastEncodedAudioFrame,
}

/* ChatMessagePacket - 11011 */
#[derive(Packet, Encodable, Decodable, EncodableWithKnownSize)]
#[packet(id = 11011, encrypted = true)]
pub struct ChatMessagePacket {
    pub message: FastString<MAX_MESSAGE_SIZE>,
}
