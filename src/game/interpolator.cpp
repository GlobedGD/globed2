#include "interpolator.hpp"

#include <util/math.hpp>
#ifdef GLOBED_DEBUG_INTERPOLATION
# include "lerp_logger.hpp"
#endif

PlayerInterpolator::PlayerInterpolator(const InterpolatorSettings& settings) : settings(settings) {
    deltaAllowance = settings.expectedDelta * 0.2f;
}

void PlayerInterpolator::addPlayer(uint32_t playerId) {
    players.emplace(playerId, PlayerState {});
#ifdef GLOBED_DEBUG_INTERPOLATION
    LerpLogger::get().reset(playerId);
#endif
}

void PlayerInterpolator::removePlayer(uint32_t playerId) {
    players.erase(playerId);
}

static inline void lerpSpecific(
    const SpecificIconData& older,
    const SpecificIconData& newer,
    SpecificIconData& output,
    float lerpDelta,
    bool isSecond
) {
    output.rotation = std::lerp(older.rotation, newer.rotation, lerpDelta);
    output.position = older.position.lerp(newer.position, lerpDelta);

    output.iconType = newer.iconType;
    output.isVisible = newer.isVisible;
    output.isLookingLeft = newer.isLookingLeft;
}

static inline PlayerInterpolator::LerpFrame extrapolateFrame(
    const PlayerInterpolator::LerpFrame& older,
    const PlayerInterpolator::LerpFrame& newer
) {
    PlayerInterpolator::LerpFrame output;

    lerpSpecific(older.visual.player1, newer.visual.player1, output.visual.player1, 2.0f, false);
    lerpSpecific(older.visual.player2, newer.visual.player2, output.visual.player2, 2.0f, true);

    // extrapolate the timestamp too
    output.timestamp = std::lerp(older.timestamp, newer.timestamp, 2.0f);

    return output;
}

void PlayerInterpolator::updatePlayer(uint32_t playerId, const PlayerData& data, float updateCounter) {
    auto& player = players.at(playerId);

    if (settings.realtime) {
        player.interpolatedState.player1 = data.player1;
        player.interpolatedState.player2 = data.player2;
        return;
    }

    // if there's a repeated frame, replace with an extrapolated one
    if (data.timestamp == player.newerFrame.timestamp) {
        auto extrapolated = extrapolateFrame(player.olderFrame, player.newerFrame);
        player.olderFrame = player.newerFrame;
        player.newerFrame = extrapolated;
        player.timeCounter = player.olderFrame.timestamp;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logExtrapolatedRealFrame(playerId, data.timestamp, extrapolated.timestamp, data.player1, extrapolated.visual.player1);
#endif

    } else if (data.timestamp - player.newerFrame.timestamp < deltaAllowance) {
        // correct the course
        player.newerFrame = data;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logRealFrame(playerId, data.timestamp, data.player1);
#endif
    } else {
        player.olderFrame = player.newerFrame;
        player.newerFrame = data;
        player.timeCounter = player.olderFrame.timestamp;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logRealFrame(playerId, data.timestamp, data.player1);
#endif
    }

}

void PlayerInterpolator::tick(float dt) {
    if (settings.realtime) return;

    for (auto& [playerId, player] : players) {
        player.timeCounter += dt; // TODO maybe dont increment on first tick?

        // time difference between 2 last real frames (most often is the same)
        float frameDiff = player.newerFrame.timestamp - player.olderFrame.timestamp;

        // if we dont have both frames yet, do nothing (maybe change behavior in future)
        if (std::isnan(frameDiff) || frameDiff <= 0.f) {
#ifdef GLOBED_DEBUG_INTERPOLATION
            LerpLogger::get().logLerpSkip(playerId, player.timeCounter, player.interpolatedState.player1);
#endif
            continue;
        }

        // time difference between current interpolated frame and the older real frame
        float diffFromOlder = player.timeCounter - player.olderFrame.timestamp;

        // time delta, usually is from 0.0 to 1.0, passed as 3rd arg to lerp(x, y, t)
        float lerpDelta = util::math::snan(diffFromOlder / frameDiff);

        lerpSpecific(player.olderFrame.visual.player1, player.newerFrame.visual.player1, player.interpolatedState.player1, lerpDelta, false);
        lerpSpecific(player.olderFrame.visual.player2, player.newerFrame.visual.player2, player.interpolatedState.player2, lerpDelta, true);

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logLerpOperation(playerId, player.timeCounter, player.interpolatedState.player1);
#endif
    };
}

const VisualPlayerState& PlayerInterpolator::getPlayerState(uint32_t playerId) {
    return players.at(playerId).interpolatedState;
}

bool PlayerInterpolator::isPlayerStale(uint32_t playerId, float lastServerPacket) {
    auto uc = players.at(playerId).updateCounter;

    return uc != 0.f && !util::math::equal(uc, lastServerPacket);
}

PlayerInterpolator::LerpFrame::LerpFrame(const PlayerData& pd) {
    timestamp = pd.timestamp;
    visual.player1 = pd.player1;
    visual.player2 = pd.player2;
}

PlayerInterpolator::LerpFrame::LerpFrame() {
    timestamp = 0.0f;
}
