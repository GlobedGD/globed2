#pragma once
#include <defs.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

class $modify(HookedLevelInfoLayer, LevelInfoLayer) {
    cocos2d::CCLabelBMFont* playerCountLabel = nullptr;

    $override
    bool init(GJGameLevel* level, bool challenge);

    $override
    void destructor();

    void updatePlayerCountLabel(uint16_t playerCount);
};
