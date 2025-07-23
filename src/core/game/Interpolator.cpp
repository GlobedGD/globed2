#include "Interpolator.hpp"
#include <globed/util/algo.hpp>

#ifdef GLOBED_DEBUG_INTERPOLATION
# define LERP_LOG(...) log::debug(__VA_ARGS__)
#else
# define LERP_LOG(...) do {} while (0)
#endif

constexpr float TIME_DRIFT_THRESHOLD = 0.20f; // 200ms

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
    state.totalFrames++;
    state.updatedAt = curTimestamp;

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
            state.oldestFrame().player1.position.x,
            state.oldestFrame().timestamp,
            state.newestFrame().player1.position.x,
            state.newestFrame().timestamp,
            player.player1.position.x,
            player.timestamp
        );
    }

    // TODO: idk some stuff with didJustJump

    // assert that the frame is newer than the last one
    if (!state.frames.empty() && player.timestamp <= state.newestFrame().timestamp) {
        LERP_LOG("[Interpolator] Frame for player {} is not newer than the last one: {} <= {}",
            player.accountId, player.timestamp, state.newestFrame().timestamp
        );
        return;
    }

    state.frames.push_back(player);

    // account for potential drift in time
    float drift = state.newestFrame().timestamp - state.timeCounter;
    if (state.frames.size() >= 2 && std::abs(drift) > TIME_DRIFT_THRESHOLD) {
        float newTs = state.frames[state.frames.size() - 2].timestamp;
        LERP_LOG("[Interpolator] Time drift for {}, resetting time from {} to {}", player.accountId, state.timeCounter, newTs);
        state.timeCounter = newTs;
    }
}

void Interpolator::updateNoop(int accountId, float curTimestamp) {
    auto& state = m_players.at(accountId);
    state.updatedAt = curTimestamp;
}

static inline void lerpSpecific(
    const PlayerObjectData& older,
    const PlayerObjectData& newer,
    PlayerObjectData& out,
    float t,
    float p1xdiff
) {
    out.copyFlagsFrom(older);

    float newGuessedX = out.position.x + p1xdiff;

    // i hate spider
    if (out.iconType == PlayerIconType::Spider && std::abs(older.position.y - newer.position.y) >= 33.f) {
        out.position.x = std::lerp(older.position.x, newer.position.x, t);
        out.position.y = older.position.y;
    } else {
        out.position = older.position.lerp(newer.position, t);
    }

    // if both us and this player are moving, try to use the guessed X position as long as it is close enough,
    // this will result in smoother movement for an FPS that is not a factor of 240
    // TODO: maybe only apply this modification if the camera is dynamic (moves with the player)
    if (std::abs(p1xdiff) > 0.1f && std::abs(older.position.x - newer.position.x) > 0.1f && std::abs(newGuessedX - out.position.x) < 3.0f) {
        LERP_LOG("[Interpolator] Rounding up X position from {} to {} for player", out.position.x, newGuessedX);
        out.position.x = newGuessedX;
    }

    // in platformer, a player may rotate by 180 degrees simply by moving left or right,
    // if that happens, do not interpolate the rotation
    if (older.isLookingLeft != newer.isLookingLeft && std::abs(normalizeAngle(newer.rotation - older.rotation)) >= 170.f) {
        out.rotation = older.rotation;
    } else {
        // rotation can wrap around, so we avoid using std::lerp
        out.rotation = lerpAngle(older.rotation, newer.rotation, t);
    }
}

static inline void lerpPlayer(
    const PlayerState& older,
    const PlayerState& newer,
    PlayerState& out,
    float t,
    float p1xdiff
) {
    out.accountId = older.accountId;
    out.timestamp = std::lerp(older.timestamp, newer.timestamp, t);
    out.frameNumber = older.frameNumber;
    out.deathCount = older.deathCount;
    out.percentage = older.percentage;
    out.isDead = older.isDead;
    out.isPaused = older.isPaused;
    out.isPracticing = older.isPracticing;
    out.isInEditor = older.isInEditor;
    out.isEditorBuilding = older.isEditorBuilding;
    out.isLastDeathReal = older.isLastDeathReal;

    lerpSpecific(older.player1, newer.player1, out.player1, t, p1xdiff);

    // only lerp player2 if present in both frames
    if (newer.player2 && older.player2) {
        out.player2 = PlayerObjectData{};
        lerpSpecific(*older.player2, *newer.player2, *out.player2, t, p1xdiff);
    }
}

void Interpolator::tick(float dt, float p1xdiff) {
    for (auto& [playerId, player] : m_players) {
        if (player.frames.size() < 2) continue;

        // TODO: increment dt here, or after processing?

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

        lerpPlayer(*older, *newer, player.interpolatedState, t, p1xdiff);

        LERP_LOG("[Interpolator] Lerp for {}: t = {}, timeCounter = {}, older ts = {}, newer ts = {}",
            playerId, t, player.timeCounter, older->timestamp, newer->timestamp
        );

        player.timeCounter += dt;
    }
}

PlayerState& Interpolator::getPlayerState(int playerId) {
    return m_players.at(playerId).interpolatedState;
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