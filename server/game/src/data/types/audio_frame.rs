use crate::data::*;

const VOICE_MAX_FRAMES_IN_AUDIO_FRAME: usize = 10;

type EncodedOpusData = Vec<u8>;

#[derive(Clone, Encodable)]
pub struct EncodedAudioFrame {
    pub opus_frames: [Option<EncodedOpusData>; VOICE_MAX_FRAMES_IN_AUDIO_FRAME],
}

/// `FastEncodedAudioFrame` requires just one heap allocation as opposed to 10.
#[derive(Clone)]
pub struct FastEncodedAudioFrame {
    pub data: Vec<u8>,
}

encode_impl!(FastEncodedAudioFrame, buf, self, {
    buf.write_bytes(&self.data);
});

decode_impl!(FastEncodedAudioFrame, buf, {
    Ok(Self {
        data: buf.read_remaining_bytes()?,
    })
});
