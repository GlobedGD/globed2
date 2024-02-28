#pragma once
#include <defs/util.hpp>
#include <defs/geode.hpp>

#include <data/types/game.hpp>

class LerpLogger : public SingletonBase<LerpLogger> {
public:
    void reset(uint32_t player);

    // real frames logging
    void logRealFrame(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);
    void logExtrapolatedRealFrame(uint32_t player, float localts, float realTime, float timeCounter, const SpecificIconData& realData, const SpecificIconData& extrapolatedData);

    // interpolated frames logging
    void logLerpOperation(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);
    void logLerpSkip(uint32_t player, float localts, float timeCounter, const SpecificIconData& data);

    void makeDump(const ghc::filesystem::path path);

private:
    struct PlayerLog;
    struct PlayerLogData;

    PlayerLog& ensureExists(uint32_t player);
    PlayerLogData makeLogData(const SpecificIconData& data, float localts, float timeCounter);

    struct PlayerLogData {
        GLOBED_ENCODE {
            buf.writeF32(localTimestamp);
            buf.writeF32(timestamp);
            buf.writePoint(position);
            buf.writeF32(rotation);
        }

        float localTimestamp;
        float timestamp;
        cocos2d::CCPoint position;
        float rotation;
    };

    struct PlayerLog {
        GLOBED_ENCODE {
            buf.writeValueVector(realFrames);

            buf.writeU32(realExtrapolatedFrames.size());
            for (const auto& [real, extp] : realExtrapolatedFrames) {
                buf.writeValue(real);
                buf.writeValue(extp);
            }

            buf.writeValueVector(lerpedFrames);
            buf.writeValueVector(lerpSkippedFrames);
        }

        std::vector<PlayerLogData> realFrames;
        std::vector<std::pair<PlayerLogData, PlayerLogData>> realExtrapolatedFrames;
        std::vector<PlayerLogData> lerpedFrames;
        std::vector<PlayerLogData> lerpSkippedFrames;

    };

    std::unordered_map<uint32_t, PlayerLog> players;
};
