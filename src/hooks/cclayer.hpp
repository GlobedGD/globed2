#pragma once
#include <defs/geode.hpp>

#ifndef GEODE_IS_WINDOWS

#include <Geode/modify/CCLayer.hpp>

class $modify(HookedCCLayer, cocos2d::CCLayer) {
    void onEnter();
    void onExit();
};

#endif // GEODE_IS_WINDOWS
