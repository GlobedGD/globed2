#pragma once

#include <defs/platform.hpp>
#include <Geode/Bindings.hpp>

class PlayerIconData;
enum class PlayerIconType : uint8_t;

namespace util::gd {
    GLOBED_DLL void reorderDownloadedLevel(GJGameLevel* level);
    GLOBED_DLL void openProfile(int accountId, int userId, const std::string& name);

    enum class Difficulty {
        Auto = -1,
        NA = 0,
        Easy = 1,
        Normal = 2,
        Hard = 3,
        Harder = 4,
        Insane = 5,
        HardDemon = 6,
        EasyDemon = 7,
        MediumDemon = 8,
        InsaneDemon = 9,
        ExtremeDemon = 10
    };

    const char* difficultyToString(Difficulty diff);
    std::optional<Difficulty> difficultyFromString(std::string_view diff);

    GLOBED_DLL Difficulty calcLevelDifficulty(GJGameLevel* level);

    int getIconWithType(const PlayerIconData& data, PlayerIconType type);
    int getIconWithType(const PlayerIconData& data, IconType type);

    enum class GameVariable {
        FastRespawn = 52,
        FastMenu = 168,
    };

    bool variable(GameVariable var);
    void setVariable(GameVariable var, bool state);

    void safePopScene();

    std::string getBaseServerUrl();
    bool isGdps();
}