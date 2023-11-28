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
* For optimization reasons, LevelDataPacket cannot be dynamically dispatched, and isn't defined here
* It is directly serialized in the packet handler in server_thread/handlers/game.rs.
*/

empty_server_packet!(LevelDataPacket, 21001);

/* PlayerListPacket - 21002 */

packet!(PlayerListPacket, 21002, false, {
    profiles: Vec<PlayerAccountData>,
});

encode_impl!(PlayerListPacket, buf, self, {
    buf.write_value_vec(&self.profiles);
});

decode_unimpl!(PlayerListPacket);

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
