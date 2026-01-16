#pragma once

#include "AudioSource.hpp"
#include <fmod.hpp>

namespace globed {

/// Represents a sound, stream, or anything else that can be represented as an FMOD::Sound
class GLOBED_DLL Sound : public AudioSource {
public:
    Sound(FMOD::Sound *sound);

    /// Creates a sound from the given path, returns error on failure.
    static Result<std::shared_ptr<Sound>> create(const std::filesystem::path &path, bool paused = false);

    /// Creates a sound from the given path, returns error on failure.
    static Result<std::shared_ptr<Sound>> create(const char *path, bool paused = false);

    /// Creates a sound from the given PCM data, returns error on failure.
    static Result<std::shared_ptr<Sound>> create(const float *pcm, size_t samples, int sampleRate, int channels,
                                                 bool paused = false);

    void rawSetVolume(float volume) override;
    void stop() override;
    bool isPlaying() const override;

protected:
    FMOD::Sound *m_sound = nullptr;
    FMOD::Channel *m_channel = nullptr;

    static Result<FMOD::Sound *> createRaw(const char *path);
    Result<> play(bool paused);
};

} // namespace globed