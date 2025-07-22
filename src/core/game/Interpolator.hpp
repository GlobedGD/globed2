#pragma once

#include <globed/core/game/PlayerState.hpp>
#include <queue>
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
    void tick(float dt);

    PlayerState& getPlayerState(int playerId);
    PlayerState& getNewerState(int playerId);
    bool isPlayerStale(int playerId, float curTimestamp);

private:
    struct LerpState {
        std::deque<PlayerState> frames;
        PlayerState interpolatedState{};
        size_t totalFrames = 0;
        std::optional<PlayerDeath> lastDeath;
        float timeCounter = -100.0f;
        float updatedAt = 0.0f;

        std::optional<PlayerDeath> takeDeath();
        PlayerState& oldestFrame();
        PlayerState& newestFrame();
    };

    std::unordered_map<int, LerpState> m_players;
};

}