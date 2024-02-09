#include "loading_layer.hpp"

#include "game_manager.hpp"

void HookedLoadingLayer::loadingFinished() {
    auto* hgm = static_cast<HookedGameManager*>(GameManager::get());
    hgm->m_fields->iconCache.clear();

    LoadingLayer::loadingFinished();


    if (Loader::get()->getLaunchFlag("globed-skip-preload") || Loader::get()->getLaunchFlag("globed-skip-death-effects")) {
        log::warn("Asset preloading is disabled. Not loading anything.");
        return;
    }

    auto& settings = GlobedSettings::get();
    if (!settings.globed.preloadAssets) return;

    util::ui::preloadAssets();
}