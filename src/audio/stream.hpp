#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "frame.hpp"
#include "sample_queue.hpp"
#include "decoder.hpp"

class AudioStream {
public:
    bool starving = false; // true if there aren't enough samples in the queue

    AudioStream(AudioDecoder&& decoder);
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
    FMOD::Channel* channel = nullptr;
    AudioSampleQueue queue;
    AudioDecoder decoder;
};


#endif // GLOBED_VOICE_SUPPORT
