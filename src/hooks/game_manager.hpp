#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GameManager.hpp>

class $modify(HookedGameManager, GameManager) {
    std::unordered_map<int, std::unordered_map<int, Ref<cocos2d::CCTexture2D>>> iconCache;

    $override
    void returnToLastScene(GJGameLevel* level);

    $override
    cocos2d::CCTexture2D* loadIcon(int iconId, int iconType, int iconRequestId);

    $override
    void reloadAllStep2();
};
