use crate::bytebufferext::*;
use crate::data::packets::*;
use crate::data::types::audio_frame::EncodedAudioFrame;

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
