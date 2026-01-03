#pragma once

#include <globed/core/data/PlayerState.hpp>
#include "SpeedTracker.hpp"
#include <deque>
#include <optional>

namespace globed {

struct OutFlags {
    std::optional<PlayerDeath> death;
    std::optional<SpiderTeleportData> spiderP1, spiderP2;
    bool jumpP1, jumpP2;
};

class Interpolator {
public:
    Interpolator() = default;

    void addPlayer(int playerId);
    void removePlayer(int playerId);
    void resetPlayer(int playerId);
    bool hasPlayer(int playerId) const;
    void updatePlayer(const PlayerState& player, float curTimestamp);
    void updateNoop(int accountId, float curTimestamp);
    void tick(float dt, cocos2d::CCPoint cameraDelta, cocos2d::CCPoint cameraVector);

    PlayerState& getPlayerState(int playerId, OutFlags& outFlags);
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

    /// Enables or disables camera corrections. When enabled, extra smoothing may be applied to players' movement using camera delta.
    /// This is useful to make players have no jitter on screen, but will interfere if camera movement isn't predictable, for example when spectating a player.
    void setCameraCorrections(bool enable);

    void setPlatformer(bool enable);

    struct LerpState {
        VectorSpeedTracker p1speedTracker;
        VectorSpeedTracker p2speedTracker;
        std::deque<PlayerState> frames;
        PlayerState interpolatedState{};
        size_t totalFrames = 0;
        std::optional<PlayerDeath> lastDeath;
        std::optional<SpiderTeleportData> lastSpiderTp1, lastSpiderTp2;
        uint8_t lastDeathCount = 0;
        float timeCounter = -100.0f;
        float lastDriftCorrection = -100.0f;
        float updatedAt = 0.0f;
        size_t hugeLagCounter = 0;

        std::optional<PlayerDeath> takeDeath();
        std::optional<SpiderTeleportData> takeSpiderTp(bool p1);
        bool takeJump(bool p1);
        PlayerState& oldestFrame();
        PlayerState& newestFrame();
    };

private:
    std::unordered_map<int, LerpState> m_players;
    size_t m_stationaryFrames = 0;
    bool m_realtime = false;
    bool m_lowLatency = false;
    bool m_platformer = false;
    bool m_cameraCorrections = true;

    bool isCameraStationary();
};

}