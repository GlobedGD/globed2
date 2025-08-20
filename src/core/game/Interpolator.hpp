#pragma once

#include <globed/core/data/PlayerState.hpp>
#include "SpeedTracker.hpp"
#include <deque>
#include <optional>

namespace globed {

struct PlayerDeath {
    bool isReal;
};

class Interpolator {
public:
    Interpolator() = default;

    void addPlayer(int playerId);
    void removePlayer(int playerId);
    bool hasPlayer(int playerId) const;
    void updatePlayer(const PlayerState& player, float curTimestamp);
    void updateNoop(int accountId, float curTimestamp);
    void tick(float dt, cocos2d::CCPoint cameraDelta, cocos2d::CCPoint cameraVector);

    PlayerState& getPlayerState(int playerId, std::optional<PlayerDeath>& outDeath);
    PlayerState& getNewerState(int playerId);
    bool isPlayerStale(int playerId, float curTimestamp);

    // settings

    /// Enables low latency mode. This will slightly increase chance of players jittering / teleporting, especially on unstable or high-ping connections,
    /// but will decrease the time between a player moving and the move being rendered on your screen.
    void setLowLatencyMode(bool enable);

    /// Enables realtime mode. This is even stronger than the low latency mode, and will cause the interpolator to always use the latest available data.
    /// One frame of delay is still present for interpolation, so the movement will still be smooth in some capacity,
    /// but lost, delayed or misplaced packets will cause jitter.
    void setRealtimeMode(bool enable);

private:
    struct LerpState {
        VectorSpeedTracker p1speedTracker;
        VectorSpeedTracker p2speedTracker;
        std::deque<PlayerState> frames;
        PlayerState interpolatedState{};
        size_t totalFrames = 0;
        std::optional<PlayerDeath> lastDeath;
        float timeCounter = -100.0f;
        float lastDriftCorrection = -100.0f;
        float updatedAt = 0.0f;

        std::optional<PlayerDeath> takeDeath();
        PlayerState& oldestFrame();
        PlayerState& newestFrame();
    };

    std::unordered_map<int, LerpState> m_players;
    size_t m_stationaryFrames = 0;
    bool m_realtime = false;
    bool m_lowLatency = false;

    bool isCameraStationary();
};

}