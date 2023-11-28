#include "audio_frame.hpp"

#if GLOBED_VOICE_SUPPORT

using namespace util::data;

EncodedAudioFrame::EncodedAudioFrame() {}

EncodedAudioFrame::~EncodedAudioFrame() {
    this->clear();
}

void EncodedAudioFrame::pushOpusFrame(EncodedOpusData&& frame) {
    if (frames.size() >= VOICE_MAX_FRAMES_IN_AUDIO_FRAME) {
        geode::log::warn("tried to push an extra frame into EncodedAudioFrame, 20 is the max");
        return;
    }

    frames.push_back(std::move(frame));
}

void EncodedAudioFrame::clear() {
    for (const auto& frame : frames) {
        OpusCodec::freeData(frame);
    }

    frames.clear();
}

size_t EncodedAudioFrame::size() const {
    return frames.size();
}

const std::vector<EncodedOpusData>& EncodedAudioFrame::getFrames() const {
    return frames;
}

void EncodedAudioFrame::encode(ByteBuffer& buf) const {
    GLOBED_REQUIRE(
        frames.size() <= VOICE_MAX_FRAMES_IN_AUDIO_FRAME,
        fmt::format("tried to encode an EncodedAudioFrame with {} frames when at most {} is permitted", frames.size(), VOICE_MAX_FRAMES_IN_AUDIO_FRAME)
    )

    // first encode all opus frames
    for (auto& frame : frames) {
        buf.writeOptionalValue<EncodedOpusData>(frame);
    }

    // if we have written less than 10, write nullopts

    for (size_t i = frames.size(); i < VOICE_MAX_FRAMES_IN_AUDIO_FRAME; i++) {
        buf.writeOptionalValue<EncodedOpusData>(std::nullopt);
    }
}

void EncodedAudioFrame::decode(ByteBuffer& buf) {
    for (size_t i = 0; i < VOICE_MAX_FRAMES_IN_AUDIO_FRAME; i++) {
        auto frame = buf.readOptionalValue<EncodedOpusData>();
        if (frame) frames.push_back(frame.value());
    }
}

#endif // GLOBED_VOICE_SUPPORT