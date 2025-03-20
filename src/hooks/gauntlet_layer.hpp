#pragma once
#include <defs/geode.hpp>

#ifndef GLOBED_DISABLE_EXTRA_HOOKS

#include <Geode/modify/GauntletLayer.hpp>

struct GLOBED_DLL HookedGauntletLayer : geode::Modify<HookedGauntletLayer, GauntletLayer> {
    struct Fields {
        std::unordered_map<int, uint16_t> levels;
        std::unordered_map<int, CCNode*> wrappers;
    };

    $override
    bool init(GauntletType type);

    $override
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2);

    void buildUI();
    void refreshPlayerCounts();
    void updatePlayerCounts(float);
};

#endif // GLOBED_DISABLE_EXTRA_HOOKS
