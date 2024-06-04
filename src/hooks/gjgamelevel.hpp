#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJGameLevel.hpp>

// revolutionary
class $modify(HookedGJGameLevel, GJGameLevel) {
    struct Fields {
        bool shouldTransitionWithPopScene = false;
    };

    $override
    void savePercentage(int, bool, int, int, bool);

    static LevelId getLevelIDFrom(GJGameLevel* level);
};
