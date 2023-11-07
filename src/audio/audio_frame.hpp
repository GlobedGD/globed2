#pragma once

#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

const size_t VOICE_TARGET_SAMPLERATE = 24000;
const size_t VOICE_TARGET_FRAMESIZE = 1440; // for opus, 60ms
const float VOICE_CHUNK_RECORD_TIME = 1.2f; // the audio buffer that will be sent in a single packet

#include "opus_codec.hpp"
#include <data/bytebuffer.hpp>

// Represents an audio frame (usually around a second long) that contains multiple opus frames
class EncodedAudioFrame {
public:
    static const size_t VOICE_OPUS_FRAMES_IN_AUDIO_FRAME =
        static_cast<size_t>(
            VOICE_CHUNK_RECORD_TIME *
            static_cast<float>(VOICE_TARGET_SAMPLERATE) / static_cast<float>(VOICE_TARGET_FRAMESIZE)
        );
    EncodedAudioFrame();
    ~EncodedAudioFrame();

    // prevent copying since we manually free the opus frames in the dtor
    EncodedAudioFrame(const EncodedAudioFrame&) = delete;
    EncodedAudioFrame operator=(const EncodedAudioFrame& other) = delete;

    // allow moving
    EncodedAudioFrame(EncodedAudioFrame&& other);

    // adds this audio frame to the list, returns 'true' if we are above the threshold of frames
    bool pushOpusFrame(EncodedOpusData&& frame);

    // extract all frames
    const std::vector<EncodedOpusData>& extractFrames() const;

    // implement Serializable

    void encode(ByteBuffer& buf) const;
    void decode(ByteBuffer& buf);

protected:
    std::vector<EncodedOpusData> frames;
};


#endif // GLOBED_VOICE_SUPPORT