#include "interpolator.hpp"

#include <util/math.hpp>
#ifdef GLOBED_DEBUG_INTERPOLATION
# include "lerp_logger.hpp"
#endif

#include <ui/hooks/play_layer.hpp>

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
    bool skipLerpX,
    bool isSecond
) {
    output.rotation = std::lerp(older.rotation, newer.rotation, lerpDelta);

    if (skipLerpX) {
        // output.position.y = std::lerp(older.position.y, newer.position.y, lerpDelta);
        output.position.y = older.position.y;
        output.position.x = older.position.x;
    } else {
        output.position = older.position.lerp(newer.position, lerpDelta);
    }

    output.iconType = newer.iconType;
    output.isVisible = newer.isVisible;
    output.isLookingLeft = newer.isLookingLeft;
}

static inline PlayerInterpolator::LerpFrame extrapolateFrame(
    const PlayerInterpolator::LerpFrame& older,
    const PlayerInterpolator::LerpFrame& newer
) {
    PlayerInterpolator::LerpFrame output;

    lerpSpecific(older.visual.player1, newer.visual.player1, output.visual.player1, 2.0f, false, false);
    lerpSpecific(older.visual.player2, newer.visual.player2, output.visual.player2, 2.0f, false, true);

    // extrapolate the timestamp too
    output.timestamp = std::lerp(older.timestamp, newer.timestamp, 2.0f);
    output.artificial = true;

    return output;
}

void PlayerInterpolator::updatePlayer(uint32_t playerId, const PlayerData& data, float updateCounter) {
    auto& player = players.at(playerId);
    player.updateCounter = updateCounter;
    player.pendingRealFrame = true;

    if (!util::math::equal(player.lastDeathTimestamp, data.lastDeathTimestamp)) {
        player.lastDeathTimestamp = data.lastDeathTimestamp;
        if (!player.firstTick) {
            player.pendingDeath = true;
        }
    }

    player.firstTick = false;

    if (settings.realtime) {
        player.interpolatedState.player1 = data.player1;
        player.interpolatedState.player2 = data.player2;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logRealFrame(playerId, this->getLocalTs(), data.timestamp, data.player1);
#endif
        return;
    }

    // if there's a repeated frame, replace with an extrapolated one
    if (EXTRAPOLATION && data.timestamp == player.newerFrame.timestamp && !player.newerFrame.artificial) {
        auto extrapolated = extrapolateFrame(player.olderFrame, player.newerFrame);
        player.olderFrame = player.newerFrame;
        player.newerFrame = extrapolated;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logExtrapolatedRealFrame(playerId, this->getLocalTs(), data.timestamp, extrapolated.timestamp, data.player1, extrapolated.visual.player1);
#endif

    } else if (data.timestamp - player.newerFrame.timestamp < deltaAllowance) {
        // correct the course
        player.newerFrame = data;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logRealFrame(playerId, this->getLocalTs(), data.timestamp, data.player1);
#endif
        return;
    } else {
        player.olderFrame = player.newerFrame;
        player.newerFrame = data;

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logRealFrame(playerId, this->getLocalTs(), data.timestamp, data.player1);
#endif
    }

    // funny stuff
    if (player.timeCounter - player.olderFrame.timestamp > settings.expectedDelta) {
        log::debug("correcting very wrong delta");
        player.timeCounter = player.olderFrame.timestamp;
    } else if (player.timeCounter - player.olderFrame.timestamp > 0.f) {
        // player.timeCounter
        // do nothing??
    } else {
        player.timeCounter = player.olderFrame.timestamp;
    }
}

void PlayerInterpolator::tick(float dt) {
    if ((volatile void*)this == nullptr) {
        log::debug("wtf player interpolator is nullptr");
    }

    if (settings.realtime) return;

    for (auto& [playerId, player] : players) {

        // time difference between 2 last real frames (most often is the same)
        float frameDiff = player.newerFrame.timestamp - player.olderFrame.timestamp;

        // if we dont have both frames yet, do nothing (maybe change behavior in future)
        if (std::isnan(frameDiff) || frameDiff <= 0.f) {
#ifdef GLOBED_DEBUG_INTERPOLATION
            LerpLogger::get().logLerpSkip(playerId, this->getLocalTs(), player.timeCounter, player.interpolatedState.player1);
#endif
            continue;
        }

        // time difference between current interpolated frame and the older real frame
        float diffFromOlder = player.timeCounter - player.olderFrame.timestamp;

        // time delta, usually is from 0.0 to 1.0, passed as 3rd arg to lerp(x1, x2, t)
        float lerpDelta = util::math::snan(diffFromOlder / frameDiff);
        bool skipLerpX = false;

        if (player.pendingRealFrame) {
            player.pendingRealFrame = false;
            skipLerpX = false;
        }

        lerpSpecific(player.olderFrame.visual.player1, player.newerFrame.visual.player1, player.interpolatedState.player1, lerpDelta, skipLerpX, false);
        lerpSpecific(player.olderFrame.visual.player2, player.newerFrame.visual.player2, player.interpolatedState.player2, lerpDelta, skipLerpX, true);

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logLerpOperation(playerId, this->getLocalTs(), player.timeCounter, player.interpolatedState.player1);
#endif

        player.timeCounter += dt;
    }
}

void PlayerInterpolator::tickWithRatio(float ratio) {
    if (settings.realtime) return;

    for (auto& [playerId, player] : players) {
        // time difference between 2 last real frames (most often is the same)
        float frameDiff = player.newerFrame.timestamp - player.olderFrame.timestamp;

        // if we dont have both frames yet, do nothing (maybe change behavior in future)
        if (std::isnan(frameDiff) || frameDiff <= 0.f) {
#ifdef GLOBED_DEBUG_INTERPOLATION
            LerpLogger::get().logLerpSkip(playerId, this->getLocalTs(), player.timeCounter, player.interpolatedState.player1);
#endif
            continue;
        }

        float dt = frameDiff / ratio;

        // time difference between current interpolated frame and the older real frame
        float diffFromOlder = player.timeCounter - player.olderFrame.timestamp;

        // time delta, usually is from 0.0 to 1.0, passed as 3rd arg to lerp(x1, x2, t)
        float lerpDelta = util::math::snan(diffFromOlder / frameDiff);
        bool skipLerpX = false;

        if (player.pendingRealFrame) {
            player.pendingRealFrame = false;
            skipLerpX = false;
        }

        lerpSpecific(player.olderFrame.visual.player1, player.newerFrame.visual.player1, player.interpolatedState.player1, lerpDelta, skipLerpX, false);
        lerpSpecific(player.olderFrame.visual.player2, player.newerFrame.visual.player2, player.interpolatedState.player2, lerpDelta, skipLerpX, true);

#ifdef GLOBED_DEBUG_INTERPOLATION
        LerpLogger::get().logLerpOperation(playerId, this->getLocalTs(), player.timeCounter, player.interpolatedState.player1);
#endif

        player.timeCounter += dt;
    }
}

VisualPlayerState& PlayerInterpolator::getPlayerState(uint32_t playerId) {
    return players.at(playerId).interpolatedState;
}

bool PlayerInterpolator::swapDeathStatus(uint32_t playerId) {
    auto& state = players.at(playerId);
    bool death = state.pendingDeath;
    state.pendingDeath = false;

    return death;
}

bool PlayerInterpolator::isPlayerStale(uint32_t playerId, float lastServerPacket) {
    auto uc = players.at(playerId).updateCounter;

    return uc != 0.f && !util::math::equal(uc, lastServerPacket);
}

float PlayerInterpolator::getLocalTs() {
    auto* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    return gpl->m_fields->timeCounter;
}

PlayerInterpolator::LerpFrame::LerpFrame(const PlayerData& pd) {
    timestamp = pd.timestamp;
    visual.player1 = pd.player1;
    visual.player2 = pd.player2;
    artificial = false;
}

PlayerInterpolator::LerpFrame::LerpFrame() {
    timestamp = 0.0f;
}
