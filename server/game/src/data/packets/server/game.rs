use crate::data::*;

/* PlayerProfilesPacket - 21000 */

packet!(PlayerProfilesPacket, 21000, false, {
    profiles: Vec<PlayerAccountData>,
});

encode_impl!(PlayerProfilesPacket, buf, self, {
    buf.write_value_vec(&self.profiles);
});

decode_unimpl!(PlayerProfilesPacket);

/* LevelDataPacket - 21001
* PlayerListPacket - 21002
* PlayerMetadataPacket - 21003
*
* For optimization reasons, these 3 cannot be dynamically dispatched, and are not defined here.
* They are encoded inline in the packet handlers in server_thread/handlers/game.rs.
*/

empty_server_packet!(LevelDataPacket, 21001);
empty_server_packet!(PlayerListPacket, 21002);
empty_server_packet!(PlayerMetadataPacket, 21003);

/* VoiceBroadcastPacket - 21010 */

packet!(VoiceBroadcastPacket, 21010, true, {
    player_id: i32,
    data: FastEncodedAudioFrame,
});

encode_impl!(VoiceBroadcastPacket, buf, self, {
    buf.write_i32(self.player_id);
    buf.write(&self.data);
});

decode_unimpl!(VoiceBroadcastPacket);

/* ChatMessageBroadcastPacket - 21011 */

packet!(ChatMessageBroadcastPacket, 21011, true, {
    player_id: i32,
    message: FastString<MAX_MESSAGE_SIZE>,
});

encode_impl!(ChatMessageBroadcastPacket, buf, self, {
    buf.write_i32(self.player_id);
    buf.write(&self.message);
});

decode_unimpl!(ChatMessageBroadcastPacket);

size_calc_impl!(ChatMessageBroadcastPacket, size_of_types!(i32, FastString<MAX_MESSAGE_SIZE>));
