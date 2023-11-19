#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "audio_frame.hpp"

class AudioSampleQueue {
public:
    void writeData(DecodedOpusData data);
    size_t copyTo(float* dest, size_t samples);
    size_t size();

private:
    std::vector<float> buf;
};

class AudioStream {
public:
    AudioStream();
    ~AudioStream();

    // prevent copying since we manually free the sound
    AudioStream(const AudioStream&) = delete;
    AudioStream operator=(const AudioStream& other) = delete;

    // allow moving
    AudioStream(AudioStream&& other) noexcept = default;
    AudioStream& operator=(AudioStream&&) noexcept = default;

    // start playing this stream
    void start();
    // write an audio frame to this stream
    void writeData(const EncodedAudioFrame& frame);

private:
    FMOD::Sound* sound = nullptr;
    AudioSampleQueue queue;
};


#endif // GLOBED_VOICE_SUPPORT