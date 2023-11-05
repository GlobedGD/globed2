#include "voice_playback_manager.hpp"

#if GLOBED_VOICE_SUPPORT

#include "audio_manager.hpp"

GLOBED_SINGLETON_DEF(VoicePlaybackManager);

VoicePlaybackManager::VoicePlaybackManager() {

}

VoicePlaybackManager::~VoicePlaybackManager() {

}

void VoicePlaybackManager::playFrame(int playerId, const EncodedAudioFrame& frame) {
    auto& vm = GlobedAudioManager::get();
    FMOD_RESULT res;

    FMOD_CREATESOUNDEXINFO exinfo;
    std::memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.length = sizeof(float) * exinfo.numchannels * (int)((float)VOICE_TARGET_SAMPLERATE * VOICE_CHUNK_RECORD_TIME);

    FMOD::Sound* sound;

    res = FMODAudioEngine::sharedEngine()->m_system->createSound(
        nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound);
    GLOBED_ASSERT(res == FMOD_OK, std::string("FMOD createSound failed: ") + std::to_string((int)res));

    // lock the sound and copy the data into it

    void* pcmData;
    res = sound->lock(0, exinfo.length, &pcmData, nullptr, nullptr, nullptr);
    GLOBED_ASSERT(res == FMOD_OK, std::string("FMOD Sound::lock failed: ") + std::to_string((int)res));

    size_t offset = 0;
    
    const auto& frames = frame.extractFrames();
    for (const auto& opusFrame : frames) {
        auto decodedFrame = vm.decodeSound(opusFrame.ptr, opusFrame.length);
        uintptr_t destPtr = reinterpret_cast<uintptr_t>(pcmData) + offset;
        std::memcpy(reinterpret_cast<void*>(destPtr), decodedFrame.ptr, decodedFrame.lengthBytes);
        offset += decodedFrame.lengthBytes;

        OpusCodec::freeData(decodedFrame);
    }

    res = sound->unlock(pcmData, nullptr, exinfo.length, 0);
    GLOBED_ASSERT(res == FMOD_OK, std::string("FMOD Sound::unlock failed: ") + std::to_string((int)res));

    vm.playSound(sound);

    // there must be a better way..
    auto msNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    activeSounds.push_back(std::make_pair(msNow, sound));

    // check for other dead sounds

    const std::chrono::milliseconds oldestPossible = 
        msNow - std::chrono::milliseconds(static_cast<int64_t>(VOICE_CHUNK_RECORD_TIME * 2000)); // 1.2s -> 2400ms

    // iterate backwards and remove dead sounds
    for (int i = activeSounds.size() - 1; i >= 0; i--) {
        const auto& [time, sound] = activeSounds[i];
        if (time < oldestPossible) {
            sound->release();
            activeSounds.erase(activeSounds.begin() + i);
        }
    }
}

#endif // GLOBED_VOICE_SUPPORT