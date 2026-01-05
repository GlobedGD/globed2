#pragma once

#include "PlayerSound.hpp"
#include "../AudioDecoder.hpp"
#include "../AudioSampleQueue.hpp"
#include "../EncodedAudioFrame.hpp"
#include "../VolumeEstimator.hpp"

#include <asp/sync.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

// This is how long it takes for audio to start playing after pcmreadcallback gets invoked
// By default, FMOD uses 400ms, we decrease it to 200ms
constexpr float AUDIO_PLAYBACK_DELAY = 0.2f;

/// Represents a sound stream of opus data coming from a player (e.g. voice chat)
class GLOBED_DLL VoiceStream : public PlayerSound {
public:
    VoiceStream(
        FMOD::Sound* sound,
        std::weak_ptr<RemotePlayer> player
    );

    ~VoiceStream();

    static Result<std::shared_ptr<VoiceStream>> create(
        std::weak_ptr<RemotePlayer> player
    );

    // write an audio frame to this stream. returns error if opus decoding failed
    Result<> writeData(const EncodedAudioFrame& frame);
    // write raw audio data to this stream
    void writeData(const float* pcm, size_t samples);

    void updateEstimator(float dt);

    void setMuted(bool muted);

    asp::time::Duration sinceLastPlayback();

    bool isStarving();
    float getAudibility() const override;
    float getVolume() const override;
    void stop() override;
    void onUpdate() override;
    void rawSetVolume(float volume) override;

private:
    AudioDecoder m_decoder;
    asp::AtomicBool m_starving = true; // true if there aren't enough samples in the queue
    asp::Mutex<AudioSampleQueue> m_queue;
    asp::Mutex<VolumeEstimator> m_estimator;
    asp::time::Instant m_lastPlaybackTime;
    asp::time::Instant m_lastUpdate;
    float m_rawVolume = 1.0f;
    bool m_muted = false;

    void readCallback(float* data, unsigned int len);
};

}