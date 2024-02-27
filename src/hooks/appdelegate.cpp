#include "appdelegate.hpp"

#include <hooks/loading_layer.hpp>
#include <net/network_manager.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

void GlobedAppDelegate::applicationDidEnterBackground() {
#ifdef GEODE_IS_ANDROID
    NetworkManager::get().suspend();
#endif

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
#ifdef GEODE_IS_ANDROID
    NetworkManager::get().resume();
#endif

    AppDelegate::applicationWillEnterForeground();
}

#ifdef GEODE_IS_MACOS
void GlobedAppDelegate::loadingIsFinished() {
    static auto* patch = util::lowlevel::nop(0x38127d, 5 + 3 + 4 + 5 + 3 + 3 + 5);
    if (!patch->isEnabled()) {
        (void) patch->enable().unwrap();
    }

    Loader::get()->queueInMainThread([patch = patch] {
        if (patch->isEnabled()) {
            (void) patch->disable().unwrap();
        }
        auto* ll = getChildOfType<LoadingLayer>(CCScene::get(), 0);
        log::debug("testing ll: {}", ll);
        if (ll) {
            ll->loadingFinished();
        }
    });
}
#endif