#pragma once

#include <globed/config.hpp>
#include <globed/core/data/PlayerIconData.hpp>

#include <Geode/Geode.hpp>

#ifdef GLOBED_BUILD
#include <cue/PlayerIcon.hpp>
#endif

#ifdef GLOBED_API_EXT_FUNCTIONS
#include <std23/move_only_function.h>
#endif

// Various gd related utilities

namespace globed {

GLOBED_DLL void reorderDownloadedLevel(GJGameLevel *level);

GLOBED_DLL void pushScene(cocos2d::CCLayer *layer);
GLOBED_DLL void pushScene(cocos2d::CCScene *scene);

GLOBED_DLL void replaceScene(cocos2d::CCLayer *layer);
GLOBED_DLL void replaceScene(cocos2d::CCScene *scene);

GLOBED_DLL void popScene();

struct GameLevelKind {
    GJGameLevel *level;

    enum { Main, Tower, Custom } kind;
};

/// Classifies a level by its ID.
/// * If it's a tower level or the challenge, the `kind` will be set to `Tower` and the `level` will be the level.
/// * If it's a main level, the `kind` will be set to `Main` and the `level` will be the level.
/// * If it's a custom level, the `kind` will be set to `Custom` and the `level` will be null.
GLOBED_DLL GameLevelKind classifyLevel(int levelId);

enum class GameVariable {
    FastRespawn = 52,
    FastMenu = 168,
};

GLOBED_DLL bool gameVariable(GameVariable var);
GLOBED_DLL void setGameVariable(GameVariable var, bool state);

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

GLOBED_DLL const char *difficultyToString(Difficulty diff);
GLOBED_DLL std::optional<Difficulty> difficultyFromString(std::string_view diff);

GLOBED_DLL Difficulty calcLevelDifficulty(GJGameLevel *level);

#ifdef GLOBED_BUILD
cue::Icons getPlayerIcons();
cue::Icons convertPlayerIcons(const PlayerIconData &data);
#endif

#ifdef GLOBED_API_EXT_FUNCTIONS

/// Gets a saved level from cache if available, otherwise fetches it.
/// The level data might not be downloaded, only the metadata.
GLOBED_DLL void getOnlineLevel(int id, std23::move_only_function<void(geode::Ref<GJGameLevel>)> cb);

#endif

} // namespace globed
