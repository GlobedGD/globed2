#include "Interpolator.hpp"
#include <globed/util/algo.hpp>

#ifdef GLOBED_DEBUG_INTERPOLATION
# define LERP_LOG(...) log::debug(__VA_ARGS__)
#else
# define LERP_LOG(...) do {} while (0)
#endif

constexpr float TIME_DRIFT_THRESHOLD = 0.20f; // 200ms
constexpr float TIME_DRIFT_SMALL_THRESHOLD = 0.05f; // 50ms
constexpr float TIME_DRIFT_SMALL_ADJ_DEADLINE = 30.0f; // 30s

using namespace geode::prelude;

namespace globed {

void Interpolator::addPlayer(int playerId) {
    m_players.emplace(playerId, LerpState{});
}

void Interpolator::removePlayer(int playerId) {
    m_players.erase(playerId);
}

bool Interpolator::hasPlayer(int playerId) const {
    return m_players.contains(playerId);
}

void Interpolator::updatePlayer(const PlayerState& player, float curTimestamp) {
    auto& state = m_players.at(player.accountId);
    state.updatedAt = curTimestamp;

    // don't push culled frames
    if (!player.player1) {
        return;
    }

    state.totalFrames++;

    if (!state.frames.empty()) {
        // ignore repeated frames
        if (player.timestamp == state.newestFrame().timestamp) {
            LERP_LOG("[Interpolator] Ignoring repeated frame for player {} at timestamp {}", player.accountId, player.timestamp);
            return;
        }

        if (player.deathCount != state.newestFrame().deathCount) {
            state.lastDeath = PlayerDeath { player.isLastDeathReal };
        }

        LERP_LOG("[Interpolator] new frame for {}, X progression: {} ({}) ... {} ({}) -> {} ({})",
            player.accountId,
            state.oldestFrame().player1->position.x,
            state.oldestFrame().timestamp,
            state.newestFrame().player1->position.x,
            state.newestFrame().timestamp,
            player.player1->position.x,
            player.timestamp
        );
    }

    // TODO: pending frame flags like jump, spider teleport

    // assert that the frame is newer than the last one
    if (!state.frames.empty() && player.timestamp <= state.newestFrame().timestamp) {
        LERP_LOG("[Interpolator] Frame for player {} is not newer than the last one: {} <= {}",
            player.accountId, player.timestamp, state.newestFrame().timestamp
        );
        return;
    }

    state.frames.push_back(player);

    // track the speed of the players
    state.p1speedTracker.pushMeasurement(curTimestamp, player.player1->position.x, player.player1->position.y);

    if (player.player2) {
        state.p2speedTracker.pushMeasurement(curTimestamp, player.player2->position.x, player.player2->position.y);
    }

    // account for potential drift in time
    if (state.frames.size() >= 2) {
        float sinceLastCorrection = state.lastDriftCorrection - state.timeCounter;

        float drift = state.newestFrame().timestamp - state.timeCounter;
        bool smallDrift = std::abs(drift) > TIME_DRIFT_SMALL_THRESHOLD;
        bool largeDrift = m_lowLatency ? smallDrift : std::abs(drift) > TIME_DRIFT_THRESHOLD;

        // we detect two kinds of drift, large and small
        // large drift always causes a correction, because the drift is big enough
        // small drift *eventually* causes a correction. for example, if you have a very stable connection,
        // but over time multiple packets just happen to get lost, the time drift will slowly creep up to the large threshold.
        // small drift detection aims to eliminate small drift at a time interval, so we check how long has it been since last drift adjustment

        bool doAdjust = largeDrift || (smallDrift && sinceLastCorrection > TIME_DRIFT_SMALL_ADJ_DEADLINE);

        // in realtime mode, always adjust. this *will* lead to poor visuals.
        if (m_realtime || doAdjust) {
            float newTs = state.frames[state.frames.size() - 2].timestamp;
            LERP_LOG("[Interpolator] Time drift for {} ({:.3f}s), resetting time from {} to {}", player.accountId, drift, state.timeCounter, newTs);
            state.timeCounter = newTs;
            state.lastDriftCorrection = state.timeCounter;
        }
    }
}

void Interpolator::updateNoop(int accountId, float curTimestamp) {
    auto& state = m_players.at(accountId);
    state.updatedAt = curTimestamp;
}

static bool areVectorsClose(const CCPoint& one, const CCPoint& two) {
    return std::abs(one.x - two.x) + std::abs(one.y - two.y) < 0.1f;
}

struct LerpContext {
    float t;
    CCPoint cameraDelta;
    CCPoint cameraVector;
    bool camStationary;
};

static inline void lerpSpecific(
    const PlayerObjectData& older,
    const PlayerObjectData& newer,
    float olderTime,
    float newerTime,
    PlayerObjectData& out,
    LerpContext& ctx,
    VectorSpeedTracker& speedTracker
) {
    out.copyFlagsFrom(older);

    CCPoint newGuessed = out.position + ctx.cameraDelta;

    // i hate spider
    if (out.iconType == PlayerIconType::Spider && std::abs(older.position.y - newer.position.y) >= 33.f) {
        out.position.x = std::lerp(older.position.x, newer.position.x, ctx.t);
        out.position.y = older.position.y;
    } else {
        out.position = older.position.lerp(newer.position, ctx.t);
    }

    // in platformer, a player may rotate by 180 degrees simply by moving left or right,
    // if that happens, do not interpolate the rotation
    if (older.isLookingLeft != newer.isLookingLeft && std::abs(normalizeAngle(newer.rotation - older.rotation)) >= 170.f) {
        out.rotation = older.rotation;
    } else {
        // rotation can wrap around, so we avoid using std::lerp
        out.rotation = lerpAngle(older.rotation, newer.rotation, ctx.t);
    }

    // calculate speed vector
    auto speedVec = speedTracker.getVector();

    // if both us and this player are moving, try to use the guessed position as long as it is close enough,
    // this will result in smoother movement for an FPS that is not a factor of 240

    constexpr float closeAllowance = 25.0f;
    constexpr float stillAllowance = 7.0f;

    bool cameraMovesX = std::fabs(ctx.cameraVector.x) > stillAllowance;
    bool cameraMovesY = std::fabs(ctx.cameraVector.y) > stillAllowance;

    bool playerMovesX = std::fabs(speedVec.first) >= stillAllowance;
    bool playerMovesY = std::fabs(speedVec.second) >= stillAllowance;

    bool similarSpeedX = std::fabs(speedVec.first - ctx.cameraVector.x) < closeAllowance;
    bool similarSpeedY = std::fabs(speedVec.second - ctx.cameraVector.y) < closeAllowance;

    float guessAllowanceX = std::fabs(ctx.cameraVector.x) / 50.f;
    float guessAllowanceY = std::fabs(ctx.cameraVector.y) / 50.f;

    LERP_LOG(
        "[Interpolator] speedVec: {}, cameraVec: {}, guessallowx: {}, newx: {}, outx: {}",
        speedVec.first,
        ctx.cameraVector.x,
        guessAllowanceX,
        newGuessed.x,
        out.position.x
    );

    if (cameraMovesX && playerMovesX && similarSpeedX && std::abs(newGuessed.x - out.position.x) < guessAllowanceX) {
        LERP_LOG("[Interpolator] Rounding up X position from {} to {} for player", out.position.x, newGuessed.x);
        out.position.x = newGuessed.x;
    }

    if (cameraMovesY && playerMovesY && similarSpeedY && std::abs(newGuessed.y - out.position.y) < guessAllowanceY) {
        LERP_LOG("[Interpolator] Rounding up Y position from {} to {} for player", out.position.y, newGuessed.y);
        out.position.y = newGuessed.y;
    }
}

static inline void lerpPlayer(
    const PlayerState& older,
    const PlayerState& newer,
    PlayerState& out,
    LerpContext& ctx,
    VectorSpeedTracker& p1spt,
    VectorSpeedTracker& p2spt
) {
    out.accountId = older.accountId;
    out.timestamp = std::lerp(older.timestamp, newer.timestamp, ctx.t);
    out.frameNumber = older.frameNumber;
    out.deathCount = older.deathCount;
    out.percentage = older.percentage;
    out.isDead = older.isDead;
    out.isPaused = older.isPaused;
    out.isPracticing = older.isPracticing;
    out.isInEditor = older.isInEditor;
    out.isEditorBuilding = older.isEditorBuilding;
    out.isLastDeathReal = older.isLastDeathReal;

    if (!out.player1) out.player1 = PlayerObjectData{};
    lerpSpecific(*older.player1, *newer.player1, older.timestamp, newer.timestamp, *out.player1, ctx, p1spt);

    // only lerp player2 if present in both frames
    if (newer.player2 && older.player2) {
        if (!out.player2) out.player2 = PlayerObjectData{};
        lerpSpecific(*older.player2, *newer.player2, older.timestamp, newer.timestamp, *out.player2, ctx, p2spt);
    }
}

void Interpolator::tick(float dt, CCPoint cameraDelta, CCPoint cameraVector) {
    if (cameraDelta.isZero()) {
        m_stationaryFrames++;
    } else {
        m_stationaryFrames = 0;
    }

    bool camStationary = this->isCameraStationary();

    for (auto& [playerId, player] : m_players) {
        if (player.frames.size() < 2) continue;

        // determine between which frames to interpolate
        PlayerState *older = nullptr, *newer = nullptr;
        for (size_t i = 0; i < player.frames.size() - 1; i++) {
            auto& a = player.frames[i];
            auto& b = player.frames[i + 1];

            if (a.timestamp <= player.timeCounter && b.timestamp > player.timeCounter) {
                older = &a;
                newer = &b;
                break;
            }
        }

        if (!older || !newer) {
            // possibly the next frame is delayed, we may need to extrapolate
            if (player.timeCounter >= player.newestFrame().timestamp) {
                // newer = &player.newestFrame();
                // older = &player.frames[player.frames.size() - 2]; // the one right before the newest

                // rather than extrapolation, wait for a new frame.
                continue;
            } else if (player.timeCounter < player.oldestFrame().timestamp) {
                older = &player.oldestFrame();
                newer = &player.frames[1]; // the one right after the oldest
            }
        }

        if (!older || !newer) {
            log::error("[Interpolator] Could not find frames to interpolate for player {}, timeCounter: {}",
                playerId, player.timeCounter
            );
            continue;
        } else if (older == newer) {
            log::error("[Interpolator] Older and newer frames are the same for player {}, timeCounter: {}",
                playerId, player.timeCounter
            );
            continue;
        }

        // pop all the frames that are older than the current older frame, as they can never be used again
        // we can assert that there will always be at least two frames in the queue after doing this
        while (&player.frames.front() != older) {
            player.frames.pop_front();
        }

        float frameDelta = newer->timestamp - older->timestamp;
        float t = (player.timeCounter - older->timestamp) / frameDelta;

        LerpContext ctx {
            t, cameraDelta, cameraVector, camStationary
        };
        lerpPlayer(*older, *newer, player.interpolatedState, ctx, player.p1speedTracker, player.p2speedTracker);

        LERP_LOG("[Interpolator] Lerp for {}: t = {}, timeCounter = {}, older ts = {}, newer ts = {}",
            playerId, t, player.timeCounter, older->timestamp, newer->timestamp
        );

        player.timeCounter += dt;
    }
}

PlayerState& Interpolator::getPlayerState(int playerId, std::optional<PlayerDeath>& outDeath) {
    auto& player = m_players.at(playerId);
    outDeath = player.takeDeath();
    return player.interpolatedState;
}

PlayerState& Interpolator::getNewerState(int playerId) {
    return m_players.at(playerId).newestFrame();
}

bool Interpolator::isPlayerStale(int playerId, float curTimestamp) {
    auto& state = m_players.at(playerId);
    if (state.totalFrames < 2) {
        return false;
    }

    return std::abs(state.updatedAt - curTimestamp) > 0.5f;
}

void Interpolator::setLowLatencyMode(bool enable) {
    m_lowLatency = enable;
}

void Interpolator::setRealtimeMode(bool enable) {
    m_realtime = enable;
}

bool Interpolator::isCameraStationary() {
    float userFps = 1.f / CCDirector::get()->getAnimationInterval();

    // fps = 0-240, return frames > 0,
    // fps = 240-480, return frames > 1,
    // fps = 480-720, return frames > 2,
    // and so on..

    // this is needed because of how gd handles physics updates, at fps higher than 240,
    // sometimes the player may not move at all, despite not being stationary.
    // this is a (silly, but functional!) formula to calculate how many frames the player has to be stationary
    // to actually be considered stationary.

    size_t frames = userFps / 240.f;
    return m_stationaryFrames > frames;
}

std::optional<PlayerDeath> Interpolator::LerpState::takeDeath() {
    std::optional<PlayerDeath> out{};
    out.swap(lastDeath);
    return out;
}

PlayerState& Interpolator::LerpState::oldestFrame() {
    return frames.front();
}

PlayerState& Interpolator::LerpState::newestFrame() {
    return frames.back();
}

}