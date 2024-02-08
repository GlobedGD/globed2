#include "voice_playback_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include "manager.hpp"

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
    if (streams.contains(playerId)) return;

    AudioDecoder decoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS);

    auto stream = std::make_unique<AudioStream>(std::move(decoder));
    stream->start();
    streams.emplace(playerId, std::move(stream));
}

void VoicePlaybackManager::removeStream(int playerId) {
    streams.erase(playerId);
}

bool VoicePlaybackManager::isSpeaking(int playerId) {
    if (!streams.contains(playerId)) {
        return false;
    }

    return !streams.at(playerId)->starving;
}

void VoicePlaybackManager::setVolume(int playerId, float volume) {
    if (!streams.contains(playerId)) {
        return;
    }

    streams.at(playerId)->setVolume(volume);
}

void VoicePlaybackManager::muteEveryone() {
    for (const auto& [playerId, stream] : streams) {
        stream->setVolume(0.f);
    }
}

void VoicePlaybackManager::setVolumeAll(float volume) {
    for (const auto& [playerId, stream] : streams) {
        stream->setVolume(volume);
    }
}

#endif // GLOBED_VOICE_SUPPORT
