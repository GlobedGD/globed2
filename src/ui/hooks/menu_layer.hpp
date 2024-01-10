#pragma once
#include <defs.hpp>

#include <Geode/modify/MenuLayer.hpp>

class $modify(HookedMenuLayer, MenuLayer) {
    bool btnActive = false;
    CCMenuItemSpriteExtra* globedBtn = nullptr;

    bool init();

    void updateGlobedButton();
    void maybeUpdateButton(float);
};