#pragma once

#include <defs/geode.hpp>
#include <Geode/modify/MoreOptionsLayer.hpp>

#ifdef GEODE_IS_ANDROID

class $modify(HookedMoreOptionsLayer, MoreOptionsLayer) {
    CCMenuItemSpriteExtra* adminBtn;

    $override
    bool init();

    $override
    void goToPage(int x);
};

#endif // GEODE_IS_ANDROID