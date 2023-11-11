#include "audio_frame.hpp"

#if GLOBED_VOICE_SUPPORT

const size_t VOICE_MAX_BYTES_IN_FRAME = 1500; // TODO ??? adjust later

using namespace util::data;

EncodedAudioFrame::EncodedAudioFrame() {}
EncodedAudioFrame::~EncodedAudioFrame() {
    for (const EncodedOpusData& frame : frames) {
        OpusCodec::freeData(frame);
    }
}

EncodedAudioFrame::EncodedAudioFrame(EncodedAudioFrame&& other) {
    this->frames = other.frames;
}

bool EncodedAudioFrame::pushOpusFrame(EncodedOpusData&& frame) {
    frames.push_back(frame);
    return frames.size() >= VOICE_OPUS_FRAMES_IN_AUDIO_FRAME;
}

const std::vector<EncodedOpusData>& EncodedAudioFrame::extractFrames() const {
    return frames;
}

void EncodedAudioFrame::encode(ByteBuffer& buf) const {
    buf.writeU16(frames.size());
    for (const EncodedOpusData& frame : frames) {
        buf.writeByteArray(frame.ptr, frame.length);
    }
}

void EncodedAudioFrame::decode(ByteBuffer& buf) {
    // when it comes to arbitrary allocation, DO NOT trust the other clients' data
    size_t length = buf.readU16();

    GLOBED_ASSERT(length < (VOICE_OPUS_FRAMES_IN_AUDIO_FRAME + 2), fmt::format("Rejecting audio packet, too many frames ({})", length));

    for (size_t i = 0; i < length; i++) {
        EncodedOpusData frame;
        size_t frameDataSize = buf.readU32();

        GLOBED_ASSERT(frameDataSize <= VOICE_MAX_BYTES_IN_FRAME, fmt::format("Rejecting audio packet, frame size too large ({})", length));

        frame.ptr = new byte[frameDataSize];
        frame.length = frameDataSize;

        buf.readBytesInto(frame.ptr, frame.length);

        frames.push_back(frame);
    }
}

#endif // GLOBED_VOICE_SUPPORT