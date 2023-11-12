use crate::bytebufferext::{decode_impl, empty_impl, encode_impl, ByteBufferExtRead, ByteBufferExtWrite};

type EncodedOpusData = Vec<u8>;
#[derive(Clone, Default)]
pub struct EncodedAudioFrame {
    pub opus_frames: Vec<EncodedOpusData>,
}

encode_impl!(EncodedAudioFrame, buf, self, {
    buf.write_u16(self.opus_frames.len() as u16);

    for frame in self.opus_frames.iter() {
        buf.write_byte_array(frame);
    }
});

empty_impl!(EncodedAudioFrame, Self::default());

decode_impl!(EncodedAudioFrame, buf, self, {
    let frames = buf.read_u16()?;
    for _ in 0..frames {
        let frame = buf.read_byte_array()?;
        self.opus_frames.push(frame);
    }
    Ok(())
});
