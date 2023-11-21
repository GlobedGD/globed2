use anyhow::bail;

use crate::bytebufferext::{decode_impl, encode_impl, ByteBufferExtRead, ByteBufferExtWrite};

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

decode_impl!(EncodedAudioFrame, buf, {
    let frames = buf.read_u16()?;
    if frames > 64 {
        bail!("failed to decode EncodedAudioFrame, way too many frames ({frames})");
    }

    let mut opus_frames = Vec::with_capacity(frames as usize);

    for _ in 0..frames {
        let frame = buf.read_byte_array()?;
        opus_frames.push(frame);
    }

    Ok(Self { opus_frames })
});
