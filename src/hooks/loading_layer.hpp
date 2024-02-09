#pragma once
#include <defs.hpp>

#include <Geode/modify/LoadingLayer.hpp>

#include <managers/settings.hpp>
#include <util/ui.hpp>

class $modify(HookedLoadingLayer, LoadingLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("LoadingLayer::loadAssets", -99999999);
    }

    void loadingFinished();
};
