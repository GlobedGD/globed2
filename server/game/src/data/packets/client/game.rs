use crate::data::*;

/* SyncIconsPacket - 11000 */

packet!(SyncIconsPacket, 11000, false, {
    icons: PlayerIconData,
});

encode_unimpl!(SyncIconsPacket);

decode_impl!(
    SyncIconsPacket,
    buf,
    Ok(Self {
        icons: buf.read_value()?
    })
);

/* RequestProfilesPacket - 11001 */

packet!(RequestProfilesPacket, 11001, false, {
    ids: Vec<i32>
});

encode_unimpl!(RequestProfilesPacket);

decode_impl!(RequestProfilesPacket, buf, {
    Ok(Self {
        ids: buf.read_value_vec()?,
    })
});

/* LevelJoinPacket - 11002 */

packet!(LevelJoinPacket, 11002, false, {
    level_id: i32
});

encode_unimpl!(LevelJoinPacket);

decode_impl!(LevelJoinPacket, buf, {
    Ok(Self {
        level_id: buf.read_i32()?,
    })
});

/* LevelLeavePacket - 11003 */

empty_client_packet!(LevelLeavePacket, 11003);

/* PlayerDataPacket - 11004 */

packet!(PlayerDataPacket, 11004, false, {
    data: PlayerData
});

encode_unimpl!(PlayerDataPacket);

decode_impl!(PlayerDataPacket, buf, Ok(Self { data: buf.read_value()? }));

/* RequestPlayerListPacket - 11005 */

empty_client_packet!(RequestPlayerListPacket, 11005);

/* VoicePacket - 11010 */

packet!(VoicePacket, 11010, true, {
    data: EncodedAudioFrame,
});

encode_unimpl!(VoicePacket);

decode_impl!(VoicePacket, buf, Ok(Self { data: buf.read_value()? }));
