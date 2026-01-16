#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <globed/config.hpp>

namespace globed {

struct GLOBED_DLL HookedMenuLayer : geode::Modify<HookedMenuLayer, MenuLayer> {
    struct Fields {
        bool btnActive = false;
        geode::Ref<CCMenuItemSpriteExtra> btn;
    };

    $override bool init();

    void recreateButton();
    void onGlobedButton(cocos2d::CCObject *);
    void checkButton(float);
};

} // namespace globed
