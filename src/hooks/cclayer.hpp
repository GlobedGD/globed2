#pragma once
#include <defs/geode.hpp>

#ifdef GEODE_IS_ANDROID

#include <Geode/modify/CCLayer.hpp>

class $modify(HookedCCLayer, cocos2d::CCLayer) {
    void onEnter();
};

#endif // GEODE_IS_ANDROID
