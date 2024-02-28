#pragma once

#include <Geode/modify/LoadingLayer.hpp>

#include <managers/settings.hpp>
#include <util/ui.hpp>
#include <util/time.hpp>

class $modify(HookedLoadingLayer, LoadingLayer) {
    int preloadingStage = 0;
    util::time::system_time_point loadingStartedTime;
    static void onModify(auto& self) {
        (void) self.setHookPriority("LoadingLayer::loadAssets", -99999999);
    }

    $override
    void loadingFinished();

    void setLabelText(const std::string_view text);
    void setLabelTextForStage();
    void finishLoading();

    void preloadingStage1(float);
    void preloadingStage2(float);
    void preloadingStage3(float);
};
