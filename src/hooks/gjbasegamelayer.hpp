#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GJBaseGameLayer.hpp>

class $modify(GlobedGJBGL, GJBaseGameLayer) {
    $override
    bool init();

    $override
    int checkCollisions(PlayerObject* player, float dt, bool p2);
};
