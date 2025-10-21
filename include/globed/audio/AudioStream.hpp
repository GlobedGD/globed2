#pragma once

#include <globed/prelude.hpp>
#include "AudioDecoder.hpp"
#include "AudioSampleQueue.hpp"
#include "EncodedAudioFrame.hpp"
#include "VolumeEstimator.hpp"
#include "VolumeLayer.hpp"

#include <asp/sync.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

class AudioStream {
public:
    AudioStream(AudioDecoder&& decoder);
    ~AudioStream();

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
    void setUserVolume(float volume, VolumeLayer globalLayer = {});
    float getUserVolume();

    void updateEstimator(float dt);
    // get how loud the sound is being played
    float getLoudness();

    asp::time::Duration sinceLastPlayback();

    bool isStarving();

private:
    asp::AtomicBool m_starving = false; // true if there aren't enough samples in the queue
    FMOD::Sound* m_sound = nullptr;
    FMOD::Channel* m_channel = nullptr;
    asp::Mutex<AudioSampleQueue> m_queue;
    AudioDecoder m_decoder;
    asp::Mutex<VolumeEstimator> m_estimator;
    asp::time::Instant m_lastPlaybackTime;
    VolumeLayer m_volume;
    VolumeLayer m_actualVolume;
};

}
