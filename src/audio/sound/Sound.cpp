#include <globed/audio/AudioManager.hpp>
#include <globed/audio/sound/Sound.hpp>

using namespace geode::prelude;

#define FMOD_UNWRAP(res) GEODE_UNWRAP(mapError(res))

namespace globed {

static auto system()
{
    return AudioManager::get().getSystem();
}

static auto mapError(FMOD_RESULT result)
{
    return AudioManager::get().mapError(result);
}

Sound::Sound(FMOD::Sound *sound) : m_sound(sound) {}

Result<std::shared_ptr<Sound>> Sound::create(const std::filesystem::path &path, bool paused)
{
    return create(utils::string::pathToString(path).c_str(), paused);
}

Result<std::shared_ptr<Sound>> Sound::create(const char *path, bool paused)
{
    auto s = std::make_shared<Sound>(GEODE_UNWRAP(createRaw(path)));
    // pre emptively register the sound so stop() gets called
    s->registerSelf(s);

    GEODE_UNWRAP(s->play(paused));
    return Ok(s);
}

Result<std::shared_ptr<Sound>> Sound::create(const float *pcm, size_t samples, int sampleRate, int channels,
                                             bool paused)
{
    FMOD_CREATESOUNDEXINFO exinfo = {};

    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels = channels;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.defaultfrequency = sampleRate;
    exinfo.length = sizeof(float) * samples;

    FMOD::Sound *sound;

    FMOD_UNWRAP(system()->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound));

    float *data;
    FMOD_UNWRAP(sound->lock(0, exinfo.length, (void **)&data, nullptr, nullptr, nullptr));

    std::memcpy(data, pcm, exinfo.length);

    FMOD_UNWRAP(sound->unlock(data, nullptr, exinfo.length, 0));

    auto s = std::make_shared<Sound>(sound);

    // pre emptively register the sound so stop() gets called
    s->registerSelf(s);

    GEODE_UNWRAP(s->play(paused));
    return Ok(s);
}

void Sound::rawSetVolume(float volume)
{
    if (m_channel) {
        m_channel->setVolume(volume);
    }
}

bool Sound::isPlaying() const
{
    if (!m_channel || !m_sound)
        return false;

    bool playing = false;
    if (FMOD_OK != m_channel->isPlaying(&playing)) {
        return false;
    }
    // TODO: not sure if this becomes false after the sound ends

    return playing;
}

void Sound::stop()
{
    if (m_channel) {
        m_channel->stop();
        m_channel = nullptr;
    }

    if (m_sound) {
        m_sound->release();
        m_sound = nullptr;
    }
}

Result<FMOD::Sound *> Sound::createRaw(const char *path)
{
    FMOD::Sound *sound = nullptr;
    auto e = system()->createSound(utils::string::pathToString(path).c_str(), FMOD_DEFAULT, nullptr, &sound);
    FMOD_UNWRAP(e);
    return Ok(sound);
}

Result<> Sound::play(bool paused)
{
    GLOBED_ASSERT(!m_channel && "Sound is already playing");

    FMOD_UNWRAP(system()->playSound(m_sound, nullptr, paused, &m_channel));

    m_channel->setVolumeRamp(true);
    return Ok();
}

} // namespace globed