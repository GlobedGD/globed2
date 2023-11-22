use crate::bytebufferext::{decode_impl, encode_impl, ByteBufferExtRead, ByteBufferExtWrite};

const VOICE_OPUS_FRAMES_IN_AUDIO_FRAME: usize = 20;

type EncodedOpusData = Vec<u8>;

#[derive(Clone)]
pub struct EncodedAudioFrame {
    pub opus_frames: [EncodedOpusData; VOICE_OPUS_FRAMES_IN_AUDIO_FRAME],
}

encode_impl!(EncodedAudioFrame, buf, self, {
    buf.write_value_array(&self.opus_frames);
});

decode_impl!(EncodedAudioFrame, buf, {
    Ok(Self {
        opus_frames: buf.read_value_array()?,
    })
});
