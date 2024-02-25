#pragma once
#include <defs.hpp>

#include <Geode/modify/LevelBrowserLayer.hpp>

#include <data/types/gd.hpp>

class $modify(HookedLevelBrowserLayer, LevelBrowserLayer) {
    std::unordered_map<LevelId, uint16_t> levels;

    $override
    void setupLevelBrowser(cocos2d::CCArray* p0);

    $override
    void destructor();

    void refreshPagePlayerCounts();
    void updatePlayerCounts(float);
};
