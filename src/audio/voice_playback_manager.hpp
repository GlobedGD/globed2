#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "audio_frame.hpp"
#include "audio_stream.hpp"
#include <chrono>

/*
* VoicePlaybackManager is responsible for playing voices of multiple people
* at the same time efficiently and without memory leaks.
* Not thread safe.
*/
class VoicePlaybackManager {
public:
    GLOBED_SINGLETON(VoicePlaybackManager);
    VoicePlaybackManager();
    ~VoicePlaybackManager();

    void playFrameStreamed(int playerId, const EncodedAudioFrame& frame);
    void stopAllStreams();

    void prepareStream(int playerId);
    void removeStream(int playerId);
    bool isSpeaking(int playerId);

private:
    std::unordered_map<int, std::unique_ptr<AudioStream>> streams;
};

#endif // GLOBED_VOICE_SUPPORT