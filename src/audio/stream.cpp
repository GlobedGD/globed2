#include "stream.hpp"

#if GLOBED_VOICE_SUPPORT

#include "manager.hpp"
#include <util/misc.hpp>

AudioStream::AudioStream(AudioDecoder&& decoder)
    : decoder(std::move(decoder)),
      estimator(std::move(VolumeEstimator(VOICE_TARGET_SAMPLERATE))) {
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.userdata = this;
    exinfo.length = sizeof(float) * exinfo.numchannels * exinfo.defaultfrequency * (VOICE_CHUNK_RECORD_TIME * 1); // TODO maybe bring back the 10 multiplier

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
        size_t copied = stream->queue.lock()->copyTo(reinterpret_cast<float*>(data), neededSamples);
        stream->estimator.lock()->feedData(reinterpret_cast<const float*>(data), copied);

        if (copied != neededSamples) {
            stream->starving = true;
            // fill the rest with the void to not repeat stuff
            for (size_t i = copied; i < neededSamples; i++) {
                ((float*)data)[i] = 0.0f;
            }
        } else {
            stream->starving = false;
            stream->lastPlaybackTime = util::time::now();
        }

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

AudioStream::AudioStream(AudioStream&& other) noexcept {
    sound = other.sound;
    channel = other.channel;
    other.sound = nullptr;
    other.channel = nullptr;

    *queue.lock() = std::move(*other.queue.lock());
    decoder = std::move(other.decoder);
    *estimator.lock() = std::move(*other.estimator.lock());
}

AudioStream& AudioStream::operator=(AudioStream&& other) noexcept {
    if (this != &other) {
        if (this->sound) {
            this->sound->release();
        }

        if (this->channel) {
            this->channel->stop();
        }

        this->sound = other.sound;
        this->channel = other.channel;

        other.sound = nullptr;
        other.channel = nullptr;

        *queue.lock() = std::move(*other.queue.lock());
        decoder = std::move(other.decoder);
        *estimator.lock() = std::move(*other.estimator.lock());
    }

    return *this;
}

void AudioStream::start() {
    if (this->channel) {
        return;
    }

    this->channel = GlobedAudioManager::get().playSound(sound);
}

Result<> AudioStream::writeData(const EncodedAudioFrame& frame) {
    const auto& frames = frame.getFrames();
    for (const auto& opusFrame : frames) {
        auto decodedFrame_ = decoder.decode(opusFrame);
        GLOBED_UNWRAP_INTO(decodedFrame_, auto decodedFrame);

        queue.lock()->writeData(decodedFrame);

        AudioDecoder::freeData(decodedFrame);
    }

    return Ok();
}

void AudioStream::writeData(const float* pcm, size_t samples) {
    queue.lock()->writeData(pcm, samples);
}

void AudioStream::setVolume(float volume) {
    if (channel) {
        channel->setVolume(volume);
    }

    this->volume = volume;
}

float AudioStream::getVolume() {
    return volume;
}

void AudioStream::updateEstimator(float dt) {
    estimator.lock()->update(dt);
}

float AudioStream::getLoudness() {
    return estimator.lock()->getVolume() * this->volume;
}

util::time::time_point AudioStream::getLastPlaybackTime() {
    return lastPlaybackTime;
}

#endif // GLOBED_VOICE_SUPPORT
