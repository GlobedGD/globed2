use crate::bytebufferext::*;
use crate::data::packets::*;
use crate::data::types::EncodedAudioFrame;
use crate::data::types::PlayerAccountData;

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
* It is directly serialized in the packet handler in server_thread.rs.
*/

empty_server_packet!(LevelDataPacket, 21001);

/* VoiceBroadcastPacket - 21010 */

packet!(VoiceBroadcastPacket, 21010, true, {
    player_id: i32,
    data: EncodedAudioFrame,
});

encode_impl!(VoiceBroadcastPacket, buf, self, {
    buf.write_i32(self.player_id);
    buf.write_value(&self.data);
});

decode_unimpl!(VoiceBroadcastPacket);
