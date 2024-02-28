#pragma once
#include <defs/platform.hpp>
#include <defs/minimal_geode.hpp>

#if GLOBED_VOICE_SUPPORT

#include "frame.hpp"
#include "sample_queue.hpp"
#include "decoder.hpp"
#include "volume_estimator.hpp"

#include <util/sync.hpp>
#include <util/time.hpp>

class AudioStream {
public:
    AudioStream(AudioDecoder&& decoder);
    ~AudioStream();

    // prevent copying since we manually free the sound
    AudioStream(const AudioStream&) = delete;
    AudioStream operator=(const AudioStream& other) = delete;

    // allow moving
    AudioStream(AudioStream&& other) noexcept;
    AudioStream& operator=(AudioStream&& other) noexcept;

    // start playing this stream
    void start();
    // write an audio frame to this stream. returns error if opus decoding failed
    Result<> writeData(const EncodedAudioFrame& frame);
    // write raw audio data to this stream
    void writeData(const float* pcm, size_t samples);

    // set the volume of the stream (0.0f - 1.0f, beyond 1.0f amplifies)
    void setVolume(float volume);

    float getVolume();

    void updateEstimator(float dt);
    // get how loud the sound is being played
    float getLoudness();

    util::time::time_point getLastPlaybackTime();

    util::sync::AtomicBool starving = false; // true if there aren't enough samples in the queue

private:
    FMOD::Sound* sound = nullptr;
    FMOD::Channel* channel = nullptr;
    util::sync::WrappingMutex<AudioSampleQueue> queue;
    AudioDecoder decoder;
    util::sync::WrappingMutex<VolumeEstimator> estimator;
    float volume = 0.f;
    util::time::time_point lastPlaybackTime;
};

#else

class AudioStream;

#endif // GLOBED_VOICE_SUPPORT
