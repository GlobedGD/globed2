#pragma once
#include <defs.hpp>

#include <Geode/modify/LevelCell.hpp>

class $modify(GlobedLevelCell, LevelCell) {
    cocos2d::CCLabelBMFont* playerCountLabel = nullptr;

    void updatePlayerCount(int count);
};
