#pragma once
#include <defs.hpp>

#include <Geode/modify/MenuLayer.hpp>

class $modify(HookedMenuLayer, MenuLayer) {
    bool btnActive = false;
    CCMenuItemSpriteExtra* globedBtn = nullptr;

    $override
    bool init();

    void updateGlobedButton();
    void maybeUpdateButton(float);
};