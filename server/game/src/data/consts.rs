/// maximum characters in a `ServerNoticePacket` or `ServerDisconnectPacket` (164)
pub const MAX_NOTICE_SIZE: usize = 164;
/// maximum characters in a user message (156)
pub const MAX_MESSAGE_SIZE: usize = 156;
/// amount of chars in a room id string (6)
pub const ROOM_ID_LENGTH: usize = 6;

// this should be the PlayerData size plus some headroom
pub const SMALL_PACKET_LIMIT: usize = 96;
