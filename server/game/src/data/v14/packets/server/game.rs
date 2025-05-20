use globed_shared::IntMap;

use crate::data::*;

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 22000, tcp = true)]
pub struct PlayerProfilesPacket {
    pub players: Vec<PlayerAccountData>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 22001, tcp = false)]
pub struct LevelDataPacket {
    pub players: Vec<AssociatedPlayerData>,
    pub custom_items: Option<IntMap<u16, i32>>,
}

#[derive(Packet, Encodable, DynamicSize)]
#[packet(id = 22002, tcp = true)]
pub struct LevelPlayerMetadataPacket {
    pub players: Vec<AssociatedPlayerMetadata>,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 22003, tcp = true)]
pub struct LevelInnerPlayerCountPacket {
    pub count: u32,
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
    pub message: InlineString<MAX_MESSAGE_SIZE>,
}

#[derive(Packet, Encodable, StaticSize)]
#[packet(id = 22012, tcp = true)]
pub struct VoiceFailedPacket {
    pub user_muted: bool,
}
