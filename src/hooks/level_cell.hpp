#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/LevelCell.hpp>

class $modify(GlobedLevelCell, LevelCell) {
    cocos2d::CCLabelBMFont* playerCountLabel = nullptr;
    cocos2d::CCSprite* playerCountIcon = nullptr;

    void updatePlayerCount(int count, bool inLists = false);
};
