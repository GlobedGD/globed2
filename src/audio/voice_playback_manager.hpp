#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "audio_frame.hpp"
#include <chrono>

/*
* VoicePlaybackManager is responsible for playing voices of multiple people
* at the same time efficiently and without memory leaks.
*/
class VoicePlaybackManager {
public:
    GLOBED_SINGLETON(VoicePlaybackManager);
    VoicePlaybackManager();
    ~VoicePlaybackManager();

    void playFrame(int playerId, const EncodedAudioFrame& frame);
    void releaseStaleSounds();

private:
    std::vector<std::pair<std::chrono::milliseconds, FMOD::Sound*>> activeSounds;
};

#endif // GLOBED_VOICE_SUPPORT