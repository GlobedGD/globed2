#pragma once

#include <globed/prelude.hpp>

namespace globed {

enum class AudioKind {
    Unknown = 0,
    VoiceChat = 1,
    EmoteSfx = 2,
    Misc = 3,
};

/// Represents a source of audio in the game
class GLOBED_DLL AudioSource {
public:
    AudioSource() = default;
    virtual ~AudioSource() = default;

    AudioSource(const AudioSource &) = delete;
    AudioSource &operator=(const AudioSource &) = delete;
    AudioSource(AudioSource &&) = delete;
    AudioSource &operator=(AudioSource &&) = delete;

    /// Stops the sound from playing. The source can assume it will not be used again after this call.
    /// Audio sources should prefer doing any cleanup here rather than in the destructor, as the destructor
    /// may be called during program exit, where cleanup could end up causing issues.
    virtual void stop() {}

    /// Should return whether the sound is still valid and playing.
    /// If a stream is simply inactive (player not talking), this should generally NOT return false, unless it's ok to
    /// cleanup the stream fully. If this returns false, the sound is completely removed from the manager.
    virtual bool isPlaying() const
    {
        return true;
    }

    /// Optional, should return estimated audibility of the audio, where 0 is inaudible and 1 is very loud.
    /// This should account for volume and other multipliers that were set by rawSetVolume
    virtual float getAudibility() const
    {
        return 1.0f;
    }

    /// Optional, should return the volume of the audio source. Recommended range is 0.0 - 1.0, but can exceed 1.0.
    /// Audio sources must use this instead of manually adjusting channel volume, or skip overriding to use volume 1.0.
    virtual float getVolume() const
    {
        return 1.0f;
    }

    AudioKind kind() const
    {
        return m_kind;
    }

    void setKind(AudioKind kind)
    {
        m_kind = kind;
    }

    virtual std::optional<CCPoint> getPosition() const
    {
        return std::nullopt;
    }

    /// This must be implemented and must set the actual volume that will be played back.
    /// Must not be called manually, AudioManager will call this with the final determined volume of the source.
    virtual void rawSetVolume(float volume) = 0;

    /// Called periodically (usually 30 times a second), right after rawSetVolume.
    /// Can be used for various custom logic the source may need.
    virtual void onUpdate() {}

protected:
    AudioKind m_kind = AudioKind::Unknown;

    /// Registers this source with AudioManager, should be called after construction.
    void registerSelf(std::shared_ptr<AudioSource> self);
};

} // namespace globed
