#pragma once
#include <defs.hpp>

#include <Geode/modify/PlayerObject.hpp>

class $modify(ComplexPlayerObject, PlayerObject) {
    // those are needed so that our changes don't impact actual PlayerObject instances
    void setRemoteState();
    bool vanilla();

    void incrementJumps();
};