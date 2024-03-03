#include "frame.hpp"

#ifdef GLOBED_VOICE_SUPPORT

using namespace util::data;

EncodedAudioFrame::EncodedAudioFrame() : _capacity(VOICE_MAX_FRAMES_IN_AUDIO_FRAME) {}
EncodedAudioFrame::EncodedAudioFrame(size_t capacity) : _capacity(capacity) {}

EncodedAudioFrame::~EncodedAudioFrame() {
    this->clear();
}

Result<> EncodedAudioFrame::pushOpusFrame(const EncodedOpusData& frame) {
    if (frames.size() >= _capacity) {
        return Err("tried to push an extra frame into EncodedAudioFrame, {} is the max", _capacity);
    }

    frames.push_back(frame);
    return Ok();
}

void EncodedAudioFrame::setCapacity(size_t frames_) {
    _capacity = frames_;
    while (frames.size() > _capacity) {
        AudioEncoder::freeData(frames.back());
        frames.pop_back();
    }
}

void EncodedAudioFrame::clear() {
    for (auto& frame : frames) {
        AudioEncoder::freeData(frame);
    }

    frames.clear();
}

size_t EncodedAudioFrame::size() const {
    return frames.size();
}

size_t EncodedAudioFrame::capacity() const {
    return _capacity;
}

const std::vector<EncodedOpusData>& EncodedAudioFrame::getFrames() const {
    return frames;
}

void EncodedAudioFrame::encode(ByteBuffer& buf) const {
    GLOBED_REQUIRE(
        frames.size() <= _capacity,
        fmt::format("tried to encode an EncodedAudioFrame with {} frames when at most {} is permitted", frames.size(), _capacity)
    )

    // first encode all opus frames
    for (auto& frame : frames) {
        buf.writeOptionalValue<EncodedOpusData>(frame);
    }

    // if we have written less than the absolute max, write nullopts

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