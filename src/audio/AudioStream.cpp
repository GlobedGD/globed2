#include <globed/audio/AudioStream.hpp>
#include <globed/audio/AudioManager.hpp>

#include <fmod_errors.h>

using namespace asp::time;

namespace globed {

AudioStream::AudioStream(AudioDecoder&& decoder)
    : m_decoder(std::move(decoder)),
      m_estimator(VolumeEstimator(VOICE_TARGET_SAMPLERATE)),
      m_lastPlaybackTime(Instant::now())
{
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.decodebuffersize = exinfo.numchannels * exinfo.defaultfrequency * AUDIO_PLAYBACK_DELAY;
    exinfo.userdata = this;
    exinfo.length = sizeof(float) * exinfo.numchannels * exinfo.defaultfrequency * 0.03f;

    exinfo.pcmreadcallback = [](FMOD_SOUND* sound_, void* data_, unsigned int len) -> FMOD_RESULT {
        FMOD::Sound* sound = reinterpret_cast<FMOD::Sound*>(sound_);
        AudioStream* stream = nullptr;
        sound->getUserData((void**)&stream);
        float* data = reinterpret_cast<float*>(data_);

        if (!stream || !data) {
            log::warn("audio stream is nullptr in cb, ignoring");
            return FMOD_OK;
        }

        // write data..

        size_t neededSamples = len / sizeof(float);
        size_t copied = stream->m_queue.lock()->readData(data, neededSamples);
        stream->m_estimator.lock()->feedData(data, copied);

        if (copied != neededSamples) {
            stream->m_starving = true;
            // fill the rest with the void to not repeat stuff
            for (size_t i = copied; i < neededSamples; i++) {
                data[i] = 0.0f;
            }
        } else {
            stream->m_starving = false;
            stream->m_lastPlaybackTime = Instant::now();
        }

        return FMOD_OK;
    };

    auto& am = AudioManager::get();

    FMOD_RESULT res;
    auto system = am.getSystem();
    res = system->createStream(nullptr, FMOD_OPENUSER | FMOD_2D | FMOD_LOOP_NORMAL, &exinfo, &m_sound);

    if (res != FMOD_OK) {
        log::error("failed to create audio stream (createStream): {}", FMOD_ErrorString(res));
    }
}

AudioStream::~AudioStream() {
    // honestly idk if this even does anything but i'm hoping it fixes a weird crash
    if (m_sound) {
        m_sound->setUserData(nullptr);
    }

    if (m_channel) {
        m_channel->stop();
    }

    if (m_sound) {
        m_sound->release();
    }
}

AudioStream::AudioStream(AudioStream&& other) noexcept {
    m_sound = other.m_sound;
    m_channel = other.m_channel;
    m_lastPlaybackTime = other.m_lastPlaybackTime;
    other.m_sound = nullptr;
    other.m_channel = nullptr;

    *m_queue.lock() = std::move(*other.m_queue.lock());
    m_decoder = std::move(other.m_decoder);
    *m_estimator.lock() = std::move(*other.m_estimator.lock());
}

AudioStream& AudioStream::operator=(AudioStream&& other) noexcept {
    if (this != &other) {
        if (m_sound) {
            m_sound->release();
        }

        if (m_channel) {
            m_channel->stop();
        }

        m_sound = other.m_sound;
        m_channel = other.m_channel;

        other.m_sound = nullptr;
        other.m_channel = nullptr;

        *m_queue.lock() = std::move(*other.m_queue.lock());
        m_decoder = std::move(other.m_decoder);
        *m_estimator.lock() = std::move(*other.m_estimator.lock());
    }

    return *this;
}

void AudioStream::start() {
    if (m_channel) {
        return;
    }

    auto res = AudioManager::get().playSound(m_sound);
    if (!res) {
        log::error("Failed to play audio stream: {}", res.unwrapErr());
        return;
    }

    m_channel = res.unwrap();
}

Result<> AudioStream::writeData(const EncodedAudioFrame& frame) {
    const auto& frames = frame.getFrames();
    for (const auto& opusFrame : frames) {
        auto decodedFrame_ = m_decoder.decode(opusFrame);
        GEODE_UNWRAP_INTO(auto decodedFrame, decodedFrame_);

        m_queue.lock()->writeData(decodedFrame.data.get(), decodedFrame.size);
    }

    return Ok();
}

void AudioStream::writeData(const float* pcm, size_t samples) {
    m_queue.lock()->writeData(pcm, samples);
}

void AudioStream::setUserVolume(float volume, VolumeLayer globalLayer) {
    m_volume = volume;
    m_actualVolume = m_volume * globalLayer;

    if (m_channel) {
        m_channel->setVolume(m_actualVolume.volume);
    }
}

float AudioStream::getUserVolume() {
    return m_volume.volume;
}

void AudioStream::updateEstimator(float dt) {
    m_estimator.lock()->update(dt);
}

float AudioStream::getLoudness() {
    float vol = m_estimator.lock()->getVolume();
    return m_actualVolume.apply(vol);
}

Duration AudioStream::sinceLastPlayback() {
    return m_lastPlaybackTime.elapsed();
}

bool AudioStream::isStarving() {
    return m_starving;
}

}
