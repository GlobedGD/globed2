#pragma once
#include <defs.hpp>

#include "visual_state.hpp"
#include <data/types/game.hpp>

struct InterpolatorSettings {
    bool realtime;      // no interpolation at all
    bool isPlatformer;  // platformer duh
};

class PlayerInterpolator {
private:
    struct PlayerState;

public:
    PlayerInterpolator(const InterpolatorSettings& settings);

    PlayerInterpolator(PlayerInterpolator&) = delete;
    PlayerInterpolator& operator=(PlayerInterpolator&) = delete;

    void addPlayer(uint32_t playerId);
    void removePlayer(uint32_t playerId);

    // Update the last known state of the player. Should be called only when new data is received.
    void updatePlayer(uint32_t playerId, const PlayerData& data, float updateCounter);

    // Interpolate the player state. Should preferrably be called every frame.
    void tick(float dt);

    // Get the current interpolated visual state of the player. This is what you pass into `RemotePlayer::updateData`
    const VisualPlayerState& getPlayerState(uint32_t playerId);

    // returns `true` if the given time of the last packet doesn't match the last update time of the player
    bool isPlayerStale(uint32_t playerId, float lastServerPacket);

private:
    std::unordered_map<uint32_t, PlayerState> players;
    InterpolatorSettings settings;

    struct PlayerState {
        float updateCounter;
        VisualPlayerState visualState;
    };
};