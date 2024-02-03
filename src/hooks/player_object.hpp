#pragma once
#include <defs.hpp>

#include <Geode/modify/PlayerObject.hpp>

class $modify(ComplexPlayerObject, PlayerObject) {
    // those are needed so that our changes don't impact actual PlayerObject instances
    bool vanilla();

    void setRemoteState();
    void setDeathEffect(int deathEffect);

    void incrementJumps();
    void playDeathEffect();

    void setPosition(const cocos2d::CCPoint&);
    const cocos2d::CCPoint& getPositionHook();
};