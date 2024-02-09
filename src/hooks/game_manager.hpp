#pragma once
#include <defs.hpp>

#include <Geode/modify/GameManager.hpp>

class $modify(HookedGameManager, GameManager) {
    std::unordered_map<int, std::unordered_map<int, cocos2d::CCTexture2D*>> iconCache;

    void returnToLastScene(GJGameLevel* level);
    cocos2d::CCTexture2D* loadIcon(int iconId, int iconType, int iconRequestId);
};
