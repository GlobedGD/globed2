#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJGameLevel.hpp>

// revolutionary
class $modify(HookedGJGameLevel, GJGameLevel) {
    bool shouldTransitionWithPopScene = false;

    $override
    void savePercentage(int, bool, int, int, bool);

    static LevelId getLevelIDFrom(GJGameLevel* level);
};
