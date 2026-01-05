#pragma once

#include "Sound.hpp"

namespace globed {

/// Represents a sound tied to a RemotePlayer.
/// The sound may either follow the target player or remain static at the position it was created at.
class GLOBED_DLL PlayerSound : public Sound {
public:
    PlayerSound(FMOD::Sound* sound, std::weak_ptr<RemotePlayer> player, CCPoint position);

    /// Creates a sound from the given path, returns error on failure.
    static Result<std::shared_ptr<PlayerSound>> create(
        const std::filesystem::path& path,
        std::shared_ptr<RemotePlayer> player,
        bool paused = false
    );

    /// Creates a sound from the given path, returns error on failure.
    static Result<std::shared_ptr<PlayerSound>> create(
        const char* path,
        std::shared_ptr<RemotePlayer> player,
        bool paused = false
    );

    std::optional<CCPoint> getPosition() const override;
    bool isPlaying() const override;

    /// Sets whether the sound should be static, and played at the position it was created at.
    /// By default is false, and the sound will follow the player.
    void setStatic(bool isStatic);

    /// Sets whether the sound should be global, and not affected by position at all.
    void setGlobal(bool isGlobal);

    /// Get the player who owns this sound.
    std::shared_ptr<RemotePlayer> getPlayer() const;

private:
    std::weak_ptr<RemotePlayer> m_player;
    CCPoint m_position;
    bool m_static = false;
    bool m_global = false;
};

}