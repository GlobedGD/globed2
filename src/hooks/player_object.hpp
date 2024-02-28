#pragma once
#include <Geode/modify/PlayerObject.hpp>

class $modify(ComplexPlayerObject, PlayerObject) {
    // those are needed so that our changes don't impact actual PlayerObject instances
    bool vanilla();

    void setRemoteState();
    void setDeathEffect(int deathEffect);

    $override
    void incrementJumps();

    $override
    void playDeathEffect();
};

// Unlike `ComplexPlayerObject`, this one is made specifically for vanilla player objects, so it is a separate $modify class.
class $modify(HookedPlayerObject, PlayerObject) {
    $override
    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to);
};
