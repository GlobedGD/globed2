#include "audio_frame.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

EncodedAudioFrame::EncodedAudioFrame() {
    // TODO remove tracking
    geode::log::debug("constructing audio frame");
}

EncodedAudioFrame::~EncodedAudioFrame() {
    // TODO remove tracking
    geode::log::debug("destructing audio frame ({} frames)", frameCount);
    for (size_t i = 0; i < frameCount; i++) {
        OpusCodec::freeData(frames[i]);
    }
}

void EncodedAudioFrame::pushOpusFrame(EncodedOpusData&& frame) {
    if (frameCount >= VOICE_OPUS_FRAMES_IN_AUDIO_FRAME) {
        geode::log::warn("tried to push an extra frame into EncodedAudioFrame, 20 is the max");
        return;
    }

    frames[frameCount++] = std::move(frame);
}

// riveting
const std::array<EncodedOpusData, EncodedAudioFrame::VOICE_OPUS_FRAMES_IN_AUDIO_FRAME>& EncodedAudioFrame::getFrames() const {
    return frames;
}

void EncodedAudioFrame::encode(ByteBuffer& buf) const {
    GLOBED_REQUIRE(
        frameCount == VOICE_OPUS_FRAMES_IN_AUDIO_FRAME,
        fmt::format("tried to encode an EncodedAudioFrame with {} frames when {} is needed", frameCount, VOICE_OPUS_FRAMES_IN_AUDIO_FRAME)
    )

    buf.writeValueArray(frames);
}

void EncodedAudioFrame::decode(ByteBuffer& buf) {
    frames = buf.readValueArray<EncodedOpusData, VOICE_OPUS_FRAMES_IN_AUDIO_FRAME>();
}

#endif // GLOBED_VOICE_SUPPORT