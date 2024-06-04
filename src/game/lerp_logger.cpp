#include "lerp_logger.hpp"

#include <defs/crash.hpp>

void LerpLogger::reset(uint32_t id) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    auto& player = this->ensureExists(id);
    player.realFrames.clear();
    player.realExtrapolatedFrames.clear();
    player.lerpedFrames.clear();
    player.lerpSkippedFrames.clear();
#endif
}

void LerpLogger::logRealFrame(uint32_t id, float localts, float timeCounter, const SpecificIconData& data) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    auto& player = this->ensureExists(id);
    player.realFrames.push_back(this->makeLogData(data, localts, timeCounter));
#endif
}

void LerpLogger::logExtrapolatedRealFrame(uint32_t id, float localts, float realTime, float timeCounter, const SpecificIconData& realData, const SpecificIconData& extrapolatedData) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    auto& player = this->ensureExists(id);
    player.realExtrapolatedFrames.push_back(std::make_pair(
        this->makeLogData(realData, localts, realTime),
        this->makeLogData(extrapolatedData, localts, timeCounter)
    ));
#endif
}

void LerpLogger::logLerpOperation(uint32_t id, float localts, float timeCounter, const SpecificIconData& data) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    auto& player = this->ensureExists(id);
    player.lerpedFrames.push_back(this->makeLogData(data, localts, timeCounter));
#endif
}

void LerpLogger::logLerpSkip(uint32_t id, float localts, float timeCounter, const SpecificIconData& data) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    auto& player = this->ensureExists(id);
    player.lerpSkippedFrames.push_back(this->makeLogData(data, localts, timeCounter));
#endif
}

PlayerLog& LerpLogger::ensureExists(uint32_t id) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    if (!players.contains(id)) {
        players.emplace(id, PlayerLog {});
    }

    return players.at(id);
#endif

    globed::unreachable(); // ensureExists is a private function, should never be callable if lerp debug is disabled
}

PlayerLogData LerpLogger::makeLogData(const SpecificIconData& data, float localts, float timeCounter) {
    return PlayerLogData {
        .localTimestamp = localts,
        .timestamp = timeCounter,
        .position = data.position,
        .rotation = data.rotation,
    };
}

void LerpLogger::makeDump(const std::filesystem::path path) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    ByteBuffer bb;

    bb.writeU32(players.size());
    for (const auto& [playerId, plog] : players) {
        bb.writeU32(playerId);
        bb.writeValue(plog);
    }

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bb.data().data()), bb.size());
    log::debug("dumped interpolation data to {} ({} bytes)", path, bb.size());
#endif
}