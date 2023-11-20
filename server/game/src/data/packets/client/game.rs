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

empty_impl!(
    SyncIconsPacket,
    Self {
        icons: PlayerIconData::default()
    }
);

decode_impl!(SyncIconsPacket, buf, self, {
    self.icons = buf.read_value()?;
    Ok(())
});

/* RequestProfilesPacket - 11001 */

packet!(RequestProfilesPacket, 11001, false, {
    ids: Vec<i32>
});

encode_unimpl!(RequestProfilesPacket);

empty_impl!(RequestProfilesPacket, Self { ids: Vec::new() });

decode_impl!(RequestProfilesPacket, buf, self, {
    let len = buf.read_u32()?;
    for _ in 0..len {
        self.ids.push(buf.read_i32()?);
    }

    Ok(())
});

/* LevelJoinPacket - 11002 */

packet!(LevelJoinPacket, 11002, false, {
    level_id: i32
});

encode_unimpl!(LevelJoinPacket);

empty_impl!(LevelJoinPacket, Self { level_id: 0 });

decode_impl!(LevelJoinPacket, buf, self, {
    self.level_id = buf.read_i32()?;
    Ok(())
});

/* LevelLeavePacket - 11003 */

empty_client_packet!(LevelLeavePacket, 11003);

/* PlayerDataPacket - 11004 */

packet!(PlayerDataPacket, 11004, false, {
    data: PlayerData
});

encode_unimpl!(PlayerDataPacket);

empty_impl!(
    PlayerDataPacket,
    Self {
        data: PlayerData::default() // :(
    }
);

decode_impl!(PlayerDataPacket, buf, self, {
    self.data = buf.read_value_fast()?;
    Ok(())
});

/* VoicePacket - 11010 */

packet!(VoicePacket, 11010, true, {
    data: EncodedAudioFrame,
});

encode_unimpl!(VoicePacket);

empty_impl!(VoicePacket, {
    Self {
        data: EncodedAudioFrame::empty(),
    }
});

decode_impl!(VoicePacket, buf, self, {
    self.data = buf.read_value()?;
    Ok(())
});
