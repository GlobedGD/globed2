use crate::data::*;

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 21000, encrypted = false)]
pub struct PlayerProfilesPacket {
    pub message: Vec<PlayerAccountData>,
}

/* LevelDataPacket - 21001
* PlayerListPacket - 21002
* PlayerMetadataPacket - 21003
*
* For optimization reasons, these 3 cannot be dynamically dispatched, and are not defined here.
* They are encoded inline in the packet handlers in server_thread/handlers/game.rs.
*/

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 21001, encrypted = false)]
pub struct LevelDataPacket {}

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 21002, encrypted = false)]
pub struct PlayerListPacket {}

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 21003, encrypted = false)]
pub struct PlayerMetadataPacket {}

#[derive(Packet, Encodable, Decodable)]
#[packet(id = 21010, encrypted = true)]
pub struct VoiceBroadcastPacket {
    pub player_id: i32,
    pub data: FastEncodedAudioFrame,
}

#[derive(Clone, Packet, Encodable, EncodableWithKnownSize, Decodable)]
#[packet(id = 21011, encrypted = true)]
pub struct ChatMessageBroadcastPacket {
    pub player_id: i32,
    pub message: FastString<MAX_MESSAGE_SIZE>,
}
