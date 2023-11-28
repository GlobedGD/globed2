#include "voice_playback_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include "audio_manager.hpp"

GLOBED_SINGLETON_DEF(VoicePlaybackManager);

VoicePlaybackManager::VoicePlaybackManager() {}

VoicePlaybackManager::~VoicePlaybackManager() {}

void VoicePlaybackManager::playFrameStreamed(int playerId, const EncodedAudioFrame& frame) {
    // if the stream doesn't exist yet, create it
    if (!streams.contains(playerId)) {
        this->prepareStream(playerId);
    }

    auto& stream = streams.at(playerId);
    stream->writeData(frame);
}

void VoicePlaybackManager::stopAllStreams() {
    streams.clear();
}

void VoicePlaybackManager::prepareStream(int playerId) {
    auto stream = std::make_unique<AudioStream>();
    stream->start();
    streams.insert(std::make_pair(playerId, std::move(stream)));
}

void VoicePlaybackManager::removeStream(int playerId) {
    streams.erase(playerId);
}

#endif // GLOBED_VOICE_SUPPORT