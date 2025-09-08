#pragma once

#include <Geode/Geode.hpp>
#include <cue/PlayerIcon.hpp>

// Various gd related utilities

namespace globed {

void reorderDownloadedLevel(GJGameLevel* level);

void pushScene(cocos2d::CCLayer* layer);
void pushScene(cocos2d::CCScene* scene);

void replaceScene(cocos2d::CCLayer* layer);
void replaceScene(cocos2d::CCScene* scene);

void popScene();

struct GameLevelKind {
    GJGameLevel* level;

    enum {
        Main,
        Tower,
        Custom
    } kind;
};

/// Classifies a level by its ID.
/// * If it's a tower level or the challenge, the `kind` will be set to `Tower` and the `level` will be the level.
/// * If it's a main level, the `kind` will be set to `Main` and the `level` will be the level.
/// * If it's a custom level, the `kind` will be set to `Custom` and the `level` will be null.
GameLevelKind classifyLevel(int levelId);

enum class GameVariable {
    FastRespawn = 52,
    FastMenu = 168,
};

bool gameVariable(GameVariable var);
void setGameVariable(GameVariable var, bool state);


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

Difficulty calcLevelDifficulty(GJGameLevel* level);

cue::Icons getPlayerIcons();

}
