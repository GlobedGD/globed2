#include <globed/audio/AudioManager.hpp>
#include <globed/audio/sound/VoiceStream.hpp>

#include <fmod_errors.h>

using namespace asp::time;

namespace globed {

VoiceStream::VoiceStream(FMOD::Sound *sound, std::weak_ptr<RemotePlayer> player)
    : PlayerSound(sound, player, {}), m_decoder(VOICE_TARGET_SAMPLERATE, VOICE_TARGET_FRAMESIZE, VOICE_CHANNELS),
      m_estimator(VolumeEstimator(VOICE_TARGET_SAMPLERATE)), m_lastPlaybackTime(Instant::now())
{
}

VoiceStream::~VoiceStream()
{
    if (m_channel) {
        m_channel->setUserData(nullptr);
    }
}

Result<std::shared_ptr<VoiceStream>> VoiceStream::create(std::weak_ptr<RemotePlayer> player)
{
    auto stream = std::make_shared<VoiceStream>(nullptr, player);

    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = 1;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = VOICE_TARGET_SAMPLERATE;
    exinfo.decodebuffersize = exinfo.numchannels * exinfo.defaultfrequency * AUDIO_PLAYBACK_DELAY;
    exinfo.userdata = stream.get();
    exinfo.length = sizeof(float) * exinfo.numchannels * exinfo.defaultfrequency * 0.03f;

    exinfo.pcmreadcallback = [](FMOD_SOUND *sound_, void *data_, unsigned int len) -> FMOD_RESULT {
        FMOD::Sound *sound = reinterpret_cast<FMOD::Sound *>(sound_);
        VoiceStream *stream = nullptr;
        sound->getUserData((void **)&stream);
        float *data = reinterpret_cast<float *>(data_);

        if (!stream || !data) {
            log::warn("audio stream is nullptr in cb, ignoring");
            return FMOD_OK;
        }

        // write data..
        stream->readCallback(data, len / sizeof(float));

        return FMOD_OK;
    };

    auto &am = AudioManager::get();
    auto system = am.getSystem();

    // pre emptively register the stream so stop() gets called
    stream->registerSelf(stream);

    GEODE_UNWRAP(am.mapError(
        system->createStream(nullptr, FMOD_OPENUSER | FMOD_2D | FMOD_LOOP_NORMAL, &exinfo, &stream->m_sound)));

    GEODE_UNWRAP(stream->play(false));

    return Ok(stream);
}

void VoiceStream::stop()
{
    PlayerSound::stop();
}

void VoiceStream::onUpdate()
{
    auto now = Instant::now();
    auto elapsed = now.durationSince(m_lastUpdate);
    m_lastUpdate = now;

    this->updateEstimator(elapsed.seconds<float>());
}

void VoiceStream::readCallback(float *data, unsigned int neededSamples)
{
    size_t copied = m_queue.lock()->readData(data, neededSamples);
    m_estimator.lock()->feedData(data, copied);

    if (copied != neededSamples) {
        m_starving = true;
        // fill the rest with the void to not repeat stuff
        for (size_t i = copied; i < neededSamples; i++) {
            data[i] = 0.0f;
        }
    } else {
        m_starving = false;
        m_lastPlaybackTime = Instant::now();
    }
}

Result<> VoiceStream::writeData(const EncodedAudioFrame &frame)
{
    const auto &frames = frame.getFrames();
    for (const auto &opusFrame : frames) {
        auto decodedFrame_ = m_decoder.decode(opusFrame);
        GEODE_UNWRAP_INTO(auto decodedFrame, decodedFrame_);

        m_queue.lock()->writeData(decodedFrame.data.get(), decodedFrame.size);
    }

    return Ok();
}

void VoiceStream::writeData(const float *pcm, size_t samples)
{
    m_queue.lock()->writeData(pcm, samples);
}

void VoiceStream::updateEstimator(float dt)
{
    m_estimator.lock()->update(dt);
}

void VoiceStream::rawSetVolume(float volume)
{
    PlayerSound::rawSetVolume(volume);
    m_rawVolume = volume;
}

float VoiceStream::getAudibility() const
{
    return m_estimator.lock()->getVolume() * m_rawVolume;
}

Duration VoiceStream::sinceLastPlayback()
{
    return m_lastPlaybackTime.elapsed();
}

void VoiceStream::setMuted(bool muted)
{
    m_muted = muted;
}

float VoiceStream::getVolume() const
{
    return m_muted ? 0.0f : PlayerSound::getVolume();
}

bool VoiceStream::isStarving()
{
    return m_starving;
}

} // namespace globed
