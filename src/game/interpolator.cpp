#include "interpolator.hpp"

#include <util/math.hpp>

PlayerInterpolator::PlayerInterpolator(const InterpolatorSettings& settings) : settings(settings) {}

void PlayerInterpolator::addPlayer(uint32_t playerId) {
    players.emplace(playerId, PlayerState {
        .updateCounter = 0.f,
    });
}

void PlayerInterpolator::removePlayer(uint32_t playerId) {
    players.erase(playerId);
}

void PlayerInterpolator::updatePlayer(uint32_t playerId, const PlayerData& data, float updateCounter) {
    auto& player = players.at(playerId);

    if (settings.realtime) {
        player.visualState.player1 = data.player1;
        player.visualState.player2 = data.player2;
        return;
    }

    // update old and new frame blah blah
}

void PlayerInterpolator::tick(float dt) {
    if (settings.realtime) return;

    // interpolate.. yay..
}

const VisualPlayerState& PlayerInterpolator::getPlayerState(uint32_t playerId) {
    return players.at(playerId).visualState;
}

bool PlayerInterpolator::isPlayerStale(uint32_t playerId, float lastServerPacket) {
    auto uc = players.at(playerId).updateCounter;

    return uc != 0.f && !util::math::equal(uc, lastServerPacket);
}
