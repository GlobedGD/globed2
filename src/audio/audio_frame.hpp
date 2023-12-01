#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "opus_codec.hpp"
#include <data/bytebuffer.hpp>

// Represents an audio frame that contains multiple encoded opus frames
class EncodedAudioFrame {
public:
    static constexpr size_t VOICE_MAX_FRAMES_IN_AUDIO_FRAME = 10;

    EncodedAudioFrame();
    ~EncodedAudioFrame();

    // prevent copying since we manually free the opus frames in the dtor
    EncodedAudioFrame(const EncodedAudioFrame&) = delete;
    EncodedAudioFrame operator=(const EncodedAudioFrame& other) = delete;

    // allow moving
    EncodedAudioFrame(EncodedAudioFrame&& other) noexcept = default;
    EncodedAudioFrame& operator=(EncodedAudioFrame&&) noexcept = default;

    // adds this audio frame to the list
    void pushOpusFrame(EncodedOpusData&& frame);

    void clear();
    size_t size() const;
    constexpr static size_t capacity() {
        return VOICE_MAX_FRAMES_IN_AUDIO_FRAME;
    }

    // extract all frames
    const std::vector<EncodedOpusData>& getFrames() const;

    // implement Serializable

    void encode(ByteBuffer& buf) const;
    void decode(ByteBuffer& buf);

protected:
    mutable std::vector<EncodedOpusData> frames;
};


#endif // GLOBED_VOICE_SUPPORT