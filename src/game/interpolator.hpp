#pragma once

#include "visual_state.hpp"
#include <data/types/game.hpp>

struct InterpolatorSettings {
    bool realtime;      // no interpolation at all
    bool isPlatformer;  // platformer duh
    float expectedDelta;
};

class PlayerInterpolator {
public:
    struct PlayerState;

    PlayerInterpolator(const InterpolatorSettings& settings);

    PlayerInterpolator(PlayerInterpolator&) = delete;
    PlayerInterpolator& operator=(PlayerInterpolator&) = delete;

    void addPlayer(int playerId);
    void removePlayer(int playerId);
    bool hasPlayer(int playerId);

    // Update the last known state of the player. Should be called only when new data is received.
    void updatePlayer(int playerId, const PlayerData& data, float updateCounter);

    // Interpolate the player state. Should preferrably be called every frame.
    void tick(float dt);

    // Get the current interpolated visual state of the player. This is what you pass into `RemotePlayer::updateData`
    VisualPlayerState& getPlayerState(int playerId);

    // returns `true` if death animation needs to be played and sets the flag back to false (so next call won't return `true` again)
    FrameFlags swapFrameFlags(int playerId);

    // returns `true` if the given time of the last packet doesn't match the last update time of the player
    bool isPlayerStale(int playerId, float lastServerPacket);

    float getLocalTs();

private:
    std::unordered_map<int, PlayerState> players;
    InterpolatorSettings settings;

    constexpr static bool EXTRAPOLATION = false;

public:

    struct LerpFrame {
        LerpFrame();
        LerpFrame(const PlayerData& pd);

        float timestamp;
        VisualPlayerState visual;
    };

    struct PlayerState {
        float updateCounter = 0.0f;
        float timeCounter = 0.0f;
        float lastDeathTimestamp = 0.0f;
        size_t totalFrames = 0;

        LerpFrame olderFrame, newerFrame;
        VisualPlayerState interpolatedState;
        bool pendingRealFrame = false;
        FrameFlags frameFlags;
    };
};