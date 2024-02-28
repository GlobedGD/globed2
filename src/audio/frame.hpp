#pragma once
#include <defs/platform.hpp>

#if GLOBED_VOICE_SUPPORT

#include "encoder.hpp"

// Represents an audio frame that contains multiple encoded opus frames
class EncodedAudioFrame {
public:
    // the absolute maximum amount of opus frames that can be encoded/decoded
    static constexpr size_t VOICE_MAX_FRAMES_IN_AUDIO_FRAME = 10;

    // the regular amount of opus frames encoded into a single EncodedAudioFrame
    static constexpr size_t LIMIT_REGULAR = VOICE_MAX_FRAMES_IN_AUDIO_FRAME;

    // the amount of opus frames encoded when the Lower Audio Latency option is enabled
    static constexpr size_t LIMIT_LOW_LATENCY = LIMIT_REGULAR / 2;

    EncodedAudioFrame();
    EncodedAudioFrame(size_t capacity);
    ~EncodedAudioFrame();

    // prevent copying since we manually free the opus frames in the dtor
    EncodedAudioFrame(const EncodedAudioFrame&) = delete;
    EncodedAudioFrame operator=(const EncodedAudioFrame& other) = delete;

    // allow moving
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

    // implement Serializable

    void encode(ByteBuffer& buf) const;
    void decode(ByteBuffer& buf);

protected:
    mutable std::vector<EncodedOpusData> frames;
    size_t _capacity;
};


#endif // GLOBED_VOICE_SUPPORT
