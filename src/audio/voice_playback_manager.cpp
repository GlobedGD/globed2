#include "voice_playback_manager.hpp"

#include "manager.hpp"

#ifdef GLOBED_VOICE_SUPPORT

Result<> VoicePlaybackManager::playFrameStreamed(int playerId, const EncodedAudioFrame& frame) {
    // if the stream doesn't exist yet, create it
    if (!streams.contains(playerId)) {
        this->prepareStream(playerId);
    }

    auto& stream = streams.at(playerId);
    return stream->writeData(frame);
}

void VoicePlaybackManager::playRawDataStreamed(int playerId, const float* pcm, size_t samples) {
    if (!streams.contains(playerId)) {
        this->prepareStream(playerId);
    }

    auto& stream = streams.at(playerId);
    stream->writeData(pcm, samples);
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

float VoicePlaybackManager::getVolume(int playerId) {
    if (!streams.contains(playerId)) {
        return 0.f;
    }

    return streams.at(playerId)->getVolume();
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

void VoicePlaybackManager::updateEstimator(int playerId, float dt) {
    if (streams.contains(playerId)) {
        streams.at(playerId)->updateEstimator(dt);
    }
}

void VoicePlaybackManager::updateAllEstimators(float dt) {
    for (const auto& [_, stream] : streams) {
        stream->updateEstimator(dt);
    }
}

float VoicePlaybackManager::getLoudness(int playerId) {
    if (!streams.contains(playerId)) return 0.f;

    return streams.at(playerId)->getLoudness();
}

util::time::time_point VoicePlaybackManager::getLastPlaybackTime(int playerId) {
    if (!streams.contains(playerId)) return {};

    return streams.at(playerId)->getLastPlaybackTime();
}

void VoicePlaybackManager::forEachStream(std::function<void(int, AudioStream&)> func) {
    for (const auto& [accountId, stream] : streams) {
        func(accountId, *stream);
    }
}

#else

void VoicePlaybackManager::playRawDataStreamed(int playerId, const float* pcm, size_t samples) {}
void VoicePlaybackManager::stopAllStreams() {}
void VoicePlaybackManager::prepareStream(int playerId) {}
void VoicePlaybackManager::removeStream(int playerId) {}
bool VoicePlaybackManager::isSpeaking(int playerId) {
    return false;
}
void VoicePlaybackManager::setVolume(int playerId, float volume) {}
float VoicePlaybackManager::getVolume(int playerId) {
    return 0.f;
}
void VoicePlaybackManager::muteEveryone() {}
void VoicePlaybackManager::setVolumeAll(float volume) {}
void VoicePlaybackManager::updateEstimator(int playerId, float dt) {}
void VoicePlaybackManager::updateAllEstimators(float dt) {}
float VoicePlaybackManager::getLoudness(int playerId) {
    return 0.f;
}
util::time::time_point VoicePlaybackManager::getLastPlaybackTime(int playerId) {
    return {};
}
void VoicePlaybackManager::forEachStream(std::function<void(int, AudioStream&)> func) {}

#endif // GLOBED_VOICE_SUPPORT
