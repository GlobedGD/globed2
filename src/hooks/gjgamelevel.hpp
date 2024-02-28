#pragma once
#include <Geode/modify/GJGameLevel.hpp>

// revolutionary
class $modify(HookedGJGameLevel, GJGameLevel) {
    bool shouldTransitionWithPopScene = false;
    bool shouldStopProgress = false; // stop progressing when using a cheat

    $override
    void savePercentage(int, bool, int, int, bool);
};
