#pragma once

#include <Geode/Geode.hpp>

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

}
