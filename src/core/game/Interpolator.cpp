#include "Interpolator.hpp"
#include <globed/util/algo.hpp>
#include <globed/core/ValueManager.hpp>

#ifdef GLOBED_DEBUG
# define LERP_LOG(...) if (::lerpDebug()) log::debug(__VA_ARGS__)
#else
# define LERP_LOG(...) do {} while (0)
#endif

constexpr float TIME_DRIFT_THRESHOLD = 0.20f; // 200ms
constexpr float TIME_DRIFT_SMALL_THRESHOLD = 0.05f; // 50ms
constexpr float TIME_DRIFT_SMALL_ADJ_DEADLINE = 30.0f; // 30s

using namespace geode::prelude;

static inline bool lerpDebug() {
    static bool val = Loader::get()->getLaunchFlag("globed/core.dev.lerp-debug");
    return val;
}

namespace globed {

void Interpolator::addPlayer(int playerId) {
    m_players.emplace(playerId, LerpState{});
}

void Interpolator::removePlayer(int playerId) {
    m_players.erase(playerId);
}

void Interpolator::resetPlayer(int playerId) {
    m_players[playerId] = LerpState{};
}

bool Interpolator::hasPlayer(int playerId) const {
    return m_players.contains(playerId);
}

static std::optional<SpiderTeleportData> detectSpiderTp(const PlayerObjectData& older, const PlayerObjectData& newer) {
    if (older.iconType != PlayerIconType::Spider || newer.iconType != PlayerIconType::Spider) {
        return std::nullopt;
    }

    float yDiff = std::abs(newer.position.y - older.position.y);
    if (yDiff < 33.f) {
        return std::nullopt;
    }

    SpiderTeleportData data;
    data.from = older.position;
    data.to = newer.position;

    return data;
}

void Interpolator::updatePlayer(const PlayerState& player, float curTimestamp) {
    auto& state = m_players.at(player.accountId);
    state.updatedAt = curTimestamp;

    bool culled = !player.player1;

    // if the player paused and tabbed out, no update packets are sent, in which case
    // the timestamp will not increase. if we just happen to join the level, the player will thus be stuck at 0,0,
    // since we will only have 1 unique frame until they unpause
    // for this reason, we forcibly ignore repeated frames if there is only 1 frame
    bool ignoreRepeated = state.frames.size() < 2;

    state.totalFrames++;

    if (!state.frames.empty()) {
        auto& prevFrame = state.newestFrame();
        // ignore repeated frames (except if there is < 2 frames)
        if (player.timestamp == prevFrame.timestamp && !ignoreRepeated) {
            LERP_LOG("[Interpolator] Ignoring repeated frame for player {} at timestamp {}", player.accountId, player.timestamp);
            return;
        }

        // detect death
        if (player.deathCount != prevFrame.deathCount) {
            state.lastDeath = PlayerDeath { player.isLastDeathReal };
        }

        if (!culled) {
            // detect spider tp
            if (auto sptp1 = detectSpiderTp(*player.player1, *prevFrame.player1)) {
                state.lastSpiderTp1 = sptp1;
            }

            if (player.player2 && prevFrame.player2) {
                if (auto sptp2 = detectSpiderTp(*player.player2, *prevFrame.player2)) {
                    state.lastSpiderTp2 = sptp2;
                }
            }

            LERP_LOG("[Interpolator] new frame for {}, X progression: {} ({}) ... {} ({}) -> {} ({})",
                player.accountId,
                state.oldestFrame().player1->position.x,
                state.oldestFrame().timestamp,
                prevFrame.player1->position.x,
                prevFrame.timestamp,
                player.player1->position.x,
                player.timestamp
            );
        }
    }

    // assert that the frame is newer than the last one
    if (!state.frames.empty() && !ignoreRepeated) {
        float timeDifference = player.timestamp - state.newestFrame().timestamp;
        if (timeDifference <= 0.f) {
            LERP_LOG("[Interpolator] Frame for player {} is not newer than the last one: {} <= {}",
                player.accountId, player.timestamp, state.newestFrame().timestamp
            );

            if (timeDifference < -1.f) {
                // more than 1 second behind, increment huge lag counter
                state.hugeLagCounter++;

                // if this happened 3 times in a row, it's very safe to assume that the player quickly rejoined the level, so reset the player state
                if (state.hugeLagCounter >= 3) {
                    this->resetPlayer(player.accountId);
                }
            }

            return;
        }
    }

    state.frames.push_back(player);

    if (!culled) {
        // track the speed of the players
        state.p1speedTracker.pushMeasurement(curTimestamp, player.player1->position.x, player.player1->position.y);

        if (player.player2) {
            state.p2speedTracker.pushMeasurement(curTimestamp, player.player2->position.x, player.player2->position.y);
        }
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
    bool platformer;
    bool cameraCorrections;
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
    auto sptp = detectSpiderTp(older, newer);

    if (sptp) {
        out.position.x = std::lerp(older.position.x, newer.position.x, ctx.t);
        out.position.y = sptp->from.y;
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

    if (ctx.cameraCorrections) {
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

    // interpolate ext data

    if (older.extData && newer.extData) {
        auto& a = *older.extData;
        auto& b = *newer.extData;

        auto ed = ExtendedPlayerData{};
        ed.velocityX = std::lerp(a.velocityX, b.velocityX, ctx.t);
        ed.velocityY = std::lerp(a.velocityY, b.velocityY, ctx.t);
        ed.accelerating = a.accelerating;
        ed.acceleration = std::lerp(a.acceleration, b.acceleration, ctx.t);
        ed.fallStartY = a.fallStartY;
        ed.isOnGround2 = a.isOnGround2;
        ed.gravityMod = a.gravityMod;
        ed.gravity = a.gravity;
        ed.touchedPad = a.touchedPad;

        out.extData = ed;
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
    out.isDead = older.isDead;
    out.isPaused = older.isPaused;
    out.isPracticing = older.isPracticing;
    out.isInEditor = older.isInEditor;
    out.isEditorBuilding = older.isEditorBuilding;
    out.isLastDeathReal = older.isLastDeathReal;

    if (!ctx.platformer) {
        out.percentage = std::lerp(older.percentage, newer.percentage, ctx.t);
    } else {
        // Percentage represents angle to the player, from 0 to 2pi, so scale the value to degrees and use lerpAngle

        // actual values returned by progress() are [0; 1], scale it up to [0; 360] degrees
        float olderAng = older.progress() * 360.f;
        float newerAng = newer.progress() * 360.f;
        float lerpedAng = lerpAngleNormalized(olderAng, newerAng, ctx.t);

        out.percentage = lerpedAng / 360.f * 65535.0;
    }

    if (newer.player1 && older.player1) {
        if (!out.player1) out.player1 = PlayerObjectData{};
        lerpSpecific(*older.player1, *newer.player1, older.timestamp, newer.timestamp, *out.player1, ctx, p1spt);
    }

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

            if (a.timestamp <= player.timeCounter && b.timestamp >= player.timeCounter) {
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
        float t = frameDelta == 0.0f ? 0.0f : (player.timeCounter - older->timestamp) / frameDelta;

        LerpContext ctx {
            t,
            cameraDelta,
            cameraVector,
            camStationary,
            m_platformer,
            m_cameraCorrections,
        };
        lerpPlayer(*older, *newer, player.interpolatedState, ctx, player.p1speedTracker, player.p2speedTracker);

        LERP_LOG("[Interpolator] Lerp for {}: t = {}, timeCounter = {}, older ts = {}, newer ts = {}",
            playerId, t, player.timeCounter, older->timestamp, newer->timestamp
        );

        player.timeCounter += dt;
    }
}

PlayerState& Interpolator::getPlayerState(int playerId, OutFlags& flags) {
    auto& player = m_players.at(playerId);

    flags.death = player.takeDeath();
    flags.spiderP1 = player.takeSpiderTp(true);
    flags.spiderP2 = player.takeSpiderTp(false);

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

void Interpolator::setCameraCorrections(bool enable) {
    m_cameraCorrections = enable;
}

void Interpolator::setPlatformer(bool enable) {
    m_platformer = enable;
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

std::optional<SpiderTeleportData> Interpolator::LerpState::takeSpiderTp(bool p1) {
    auto& in = p1 ? lastSpiderTp1 : lastSpiderTp2;
    std::optional<SpiderTeleportData> out{};
    out.swap(in);
    return out;
}

PlayerState& Interpolator::LerpState::oldestFrame() {
    return frames.front();
}

PlayerState& Interpolator::LerpState::newestFrame() {
    return frames.back();
}

}