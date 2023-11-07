#include "voice_playback_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include "audio_manager.hpp"

GLOBED_SINGLETON_DEF(VoicePlaybackManager);

VoicePlaybackManager::VoicePlaybackManager() {}

VoicePlaybackManager::~VoicePlaybackManager() {}

void VoicePlaybackManager::playFrameStreamed(int playerId, const EncodedAudioFrame& frame) {
    // if the stream doesn't exist yet, create it
    if (!streams.contains(playerId)) {
        auto stream = std::make_unique<AudioStream>();
        stream->start();
        streams.insert(std::make_pair(playerId, std::move(stream)));
    }

    auto& stream = streams.at(playerId);
    stream->writeData(frame);
}

#endif // GLOBED_VOICE_SUPPORT