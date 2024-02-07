#pragma once
#include <defs.hpp>

#include <Geode/modify/GameManager.hpp>

class $modify(HookedGameManager, GameManager) {
    void returnToLastScene(GJGameLevel* level);
};
