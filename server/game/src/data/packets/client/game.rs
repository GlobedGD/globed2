use crate::bytebufferext::*;
use crate::data::packets::*;
use crate::data::types::EncodedAudioFrame;
use crate::data::types::PlayerData;
use crate::data::types::PlayerIconData;

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
    let len = buf.read_u32()?;
    let mut ids = Vec::new();
    for _ in 0..len {
        ids.push(buf.read_i32()?);
    }

    Ok(Self { ids })
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

/* VoicePacket - 11010 */

packet!(VoicePacket, 11010, true, {
    data: EncodedAudioFrame,
});

encode_unimpl!(VoicePacket);

decode_impl!(VoicePacket, buf, Ok(Self { data: buf.read_value()? }));
