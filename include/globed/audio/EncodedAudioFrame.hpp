#pragma once

#include <globed/prelude.hpp>
#include "AudioEncoder.hpp"

namespace globed {

struct EncodedAudioFrame {
    // the absolute maximum amount of opus frames that can be encoded/decoded
    static constexpr size_t VOICE_MAX_FRAMES_IN_AUDIO_FRAME = 10;

    // the regular amount of opus frames encoded into a single EncodedAudioFrame
    static constexpr size_t LIMIT_REGULAR = VOICE_MAX_FRAMES_IN_AUDIO_FRAME;

    // the amount of opus frames encoded when the Lower Audio Latency option is enabled
    static constexpr size_t LIMIT_LOW_LATENCY = LIMIT_REGULAR / 2;

    EncodedAudioFrame();
    EncodedAudioFrame(size_t capacity);
    ~EncodedAudioFrame();

    EncodedAudioFrame(const EncodedAudioFrame&) = default;
    EncodedAudioFrame& operator=(const EncodedAudioFrame& other) = default;
    EncodedAudioFrame(EncodedAudioFrame&& other) noexcept = default;
    EncodedAudioFrame& operator=(EncodedAudioFrame&&) noexcept = default;

    // adds this audio frame to the list
    Result<> pushOpusFrame(const EncodedOpusData& frame);

    // set the capacity of the audio frame, in individual opus frames
    void setCapacity(size_t frames);

    void clear();
    size_t size() const;
    size_t capacity() const;

    // extract all frames
    const std::vector<EncodedOpusData>& getFrames() const;

protected:
    std::vector<EncodedOpusData> m_frames;
    size_t m_capacity;
};

}