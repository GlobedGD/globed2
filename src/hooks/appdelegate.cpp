#include "appdelegate.hpp"

#include <net/network_manager.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

static inline util::time::system_time_point suspendedAt;

#ifdef GEODE_IS_ANDROID
void GlobedAppDelegate::applicationDidEnterBackground() {
    NetworkManager::get().suspend();
    suspendedAt = util::time::systemNow();

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    NetworkManager::get().resume();
    auto passed = util::time::systemNow() - suspendedAt;
    if (passed > util::time::seconds(60)) {
        // to avoid firing twice
        suspendedAt = util::time::systemNow();
        NetworkManager::get().disconnectWithMessage("connection lost, application was in the background for too long");
    }

    AppDelegate::applicationWillEnterForeground();
}
#endif // GEODE_IS_ANDROID

#ifdef GEODE_IS_MACOS
void GlobedAppDelegate::loadingIsFinished() {
    AppDelegate::loadingIsFinished();

    // LoadingLayer::loadingFinished is inlined, so we hook a function that gets called before that (AppDelegate::loadingIsFinished),
    // nop out the inlined code (so the transition to menulayer doesn't happen), then call our loadingFinished hook on the next frame.
    static auto* patch = util::lowlevel::nop(0x38127d, 5 + 3 + 4 + 5 + 3 + 3 + 5);
    if (!patch->isEnabled()) {
        (void) patch->enable().unwrap();
    }

    Loader::get()->queueInMainThread([patch = patch] {
        if (patch->isEnabled()) {
            (void) patch->disable().unwrap();
        }
        auto* ll = getChildOfType<LoadingLayer>(CCScene::get(), 0);
        if (ll) {
            ll->loadingFinished();
        }
    });
}
#endif // GEODE_IS_MACOS