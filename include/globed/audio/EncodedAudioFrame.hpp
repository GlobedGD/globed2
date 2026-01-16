#pragma once

#include "AudioEncoder.hpp"
#include <globed/prelude.hpp>

namespace globed {

struct GLOBED_DLL EncodedAudioFrame {
    // the absolute maximum amount of opus frames that can be encoded/decoded
    static constexpr size_t VOICE_MAX_FRAMES_IN_AUDIO_FRAME = 10;

    EncodedAudioFrame();
    EncodedAudioFrame(size_t capacity);
    ~EncodedAudioFrame();

    EncodedAudioFrame(const EncodedAudioFrame &) = default;
    EncodedAudioFrame &operator=(const EncodedAudioFrame &other) = default;
    EncodedAudioFrame(EncodedAudioFrame &&other) noexcept = default;
    EncodedAudioFrame &operator=(EncodedAudioFrame &&) noexcept = default;

    // adds this audio frame to the list
    Result<> pushOpusFrame(const EncodedOpusData &frame);

    // set the capacity of the audio frame, in individual opus frames
    void setCapacity(size_t frames);

    void clear();
    size_t size() const;
    size_t capacity() const;

    // extract all frames
    const std::vector<EncodedOpusData> &getFrames() const;

protected:
    std::vector<EncodedOpusData> m_frames;
    size_t m_capacity;
};

} // namespace globed