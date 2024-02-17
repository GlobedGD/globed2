#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "stream.hpp"

/*
* VoicePlaybackManager is responsible for playing voices of multiple people
* at the same time efficiently and without memory leaks (?).
* Not thread safe.
*/
class VoicePlaybackManager : public SingletonBase<VoicePlaybackManager> {
public:
    void playFrameStreamed(int playerId, const EncodedAudioFrame& frame);
    void playRawDataStreamed(int playerId, const float* pcm, size_t samples);
    void stopAllStreams();

    void prepareStream(int playerId);
    void removeStream(int playerId);
    bool isSpeaking(int playerId);
    void setVolume(int playerId, float volume);
    float getVolume(int playerId);
    void muteEveryone();
    void setVolumeAll(float volume);

    void updateEstimator(int playerId, float dt);
    void updateAllEstimators(float dt);

    float getLoudness(int playerId);

private:
    std::unordered_map<int, std::unique_ptr<AudioStream>> streams;
};

#endif // GLOBED_VOICE_SUPPORT
