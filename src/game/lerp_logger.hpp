#pragma once
#include <defs/geode.hpp>

#include <filesystem>

#include <data/types/game.hpp>
#include <util/singleton.hpp>

struct PlayerLogData {
    float localTimestamp;
    float timestamp;
    cocos2d::CCPoint position;
    float rotation;
};

struct PlayerLog {
    std::vector<PlayerLogData> realFrames;
    std::vector<std::pair<PlayerLogData, PlayerLogData>> realExtrapolatedFrames;
    std::vector<PlayerLogData> lerpedFrames;
    std::vector<PlayerLogData> lerpSkippedFrames;
};

GLOBED_SERIALIZABLE_STRUCT(PlayerLogData, (localTimestamp, timestamp, position, rotation));
GLOBED_SERIALIZABLE_STRUCT(PlayerLog, (realFrames, realExtrapolatedFrames, lerpedFrames, lerpSkippedFrames));

class LerpLogger : public SingletonBase<LerpLogger> {
public:
    void reset(uint32_t player);

    // real frames logging
    void logRealFrame(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);
    void logExtrapolatedRealFrame(uint32_t player, float localts, float realTime, float timeCounter, const SpecificIconData& realData, const SpecificIconData& extrapolatedData);

    // interpolated frames logging
    void logLerpOperation(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);
    void logLerpSkip(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);

    void makeDump(const std::filesystem::path path);

private:
    PlayerLog& ensureExists(uint32_t player);
    PlayerLogData makeLogData(const SpecificIconData& data, float localts, float timeCounter);

    std::unordered_map<uint32_t, PlayerLog> players;
};

