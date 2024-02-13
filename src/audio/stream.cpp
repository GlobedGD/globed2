#include "stream.hpp"

#if GLOBED_VOICE_SUPPORT

#include "manager.hpp"
#include <util/misc.hpp>

AudioStream::AudioStream(AudioDecoder&& decoder) : decoder(std::move(decoder)) {
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.userdata = this;
    exinfo.length = sizeof(float) * exinfo.numchannels * exinfo.defaultfrequency * (VOICE_CHUNK_RECORD_TIME * EncodedAudioFrame::VOICE_MAX_FRAMES_IN_AUDIO_FRAME);

    // log::debug("{}: creating stream, length: {}", util::time::nowPretty(), exinfo.length);
    exinfo.pcmreadcallback = [](FMOD_SOUND* sound_, void* data, unsigned int len) -> FMOD_RESULT {
        FMOD::Sound* sound = reinterpret_cast<FMOD::Sound*>(sound_);
        AudioStream* stream = nullptr;
        sound->getUserData((void**)&stream);

        if (!stream || !data) {
            log::debug("audio stream is nullptr in cb, ignoring");
            return FMOD_OK;
        }

        // write data..

        size_t neededSamples = len / sizeof(float);
        size_t copied = stream->queue.copyTo(reinterpret_cast<float*>(data), neededSamples);

        if (copied != neededSamples) {
            stream->starving = true;
            // fill the rest with the void to not repeat stuff
            for (size_t i = copied; i < neededSamples; i++) {
                ((float*)data)[i] = 0.0f;
            }
        } else {
            stream->starving = false;
        }

        float pcmVolume = util::misc::calculatePcmVolume(reinterpret_cast<const float*>(data), neededSamples);
        stream->loudness = stream->volume * pcmVolume;

        return FMOD_OK;
    };

    auto& vm = GlobedAudioManager::get();

    FMOD_RESULT res;
    auto system = vm.getSystem();
    res = system->createStream(nullptr, FMOD_OPENUSER | FMOD_2D | FMOD_LOOP_NORMAL, &exinfo, &sound);

    GLOBED_REQUIRE(res == FMOD_OK, GlobedAudioManager::formatFmodError(res, "System::createStream"))
}

AudioStream::~AudioStream() {
    // honestly idk if this even does anything but i'm hoping it fixes a weird crash
    if (sound) {
        sound->setUserData(nullptr);
    }

    if (channel) {
        channel->stop();
    }

    if (sound) {
        sound->release();
    }
}

void AudioStream::start() {
    if (this->channel) {
        return;
    }

    this->channel = GlobedAudioManager::get().playSound(sound);
}

void AudioStream::writeData(const EncodedAudioFrame& frame) {
    const auto& frames = frame.getFrames();
    for (const auto& opusFrame : frames) {
        auto decodedFrame = decoder.decode(opusFrame);
        queue.writeData(decodedFrame);

        AudioDecoder::freeData(decodedFrame);
    }
}

void AudioStream::writeData(const float* pcm, size_t samples) {
    queue.writeData(pcm, samples);
}

void AudioStream::setVolume(float volume) {
    if (channel) {
        channel->setVolume(volume);
    }

    this->volume = volume;
}

float AudioStream::getLoudness() {
    return loudness;
}

#endif // GLOBED_VOICE_SUPPORT
