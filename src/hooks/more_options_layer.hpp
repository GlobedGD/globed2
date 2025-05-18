#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/MoreOptionsLayer.hpp>

struct GLOBED_DLL HookedMoreOptionsLayer : geode::Modify<HookedMoreOptionsLayer, MoreOptionsLayer> {
    struct Fields {
        Ref<CCMenuItemSpriteExtra> adminBtn = nullptr;
    };

    $override
    bool init();

    $override
    void goToPage(int x);
};
