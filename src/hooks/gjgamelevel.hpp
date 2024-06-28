#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJGameLevel.hpp>

// revolutionary
class $modify(HookedGJGameLevel, GJGameLevel) {
    struct Fields {
        bool shouldTransitionWithPopScene = false;
    };

#ifndef GEODE_IS_ARM_MAC
    $override
    void savePercentage(int, bool, int, int, bool);
#endif

    static LevelId getLevelIDFrom(GJGameLevel* level);
};
