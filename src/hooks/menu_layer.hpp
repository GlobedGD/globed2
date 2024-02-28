#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/MenuLayer.hpp>

class $modify(HookedMenuLayer, MenuLayer) {
    bool btnActive = false;
    Ref<CCMenuItemSpriteExtra> globedBtn = nullptr;

    $override
    bool init();

    void updateGlobedButton();
    void maybeUpdateButton(float);
};