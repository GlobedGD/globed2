#include "loading_layer.hpp"

void HookedLoadingLayer::loadingFinished() {
    LoadingLayer::loadingFinished();

    if (Loader::get()->getLaunchFlag("globed-skip-preload") || Loader::get()->getLaunchFlag("globed-skip-death-effects")) {
        log::warn("Asset preloading is disabled. Not loading anything.");
        return;
    }

    auto& settings = GlobedSettings::get();
    if (!settings.globed.preloadAssets) return;

    util::ui::preloadAssets();
}