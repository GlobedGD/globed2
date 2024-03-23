#include "appdelegate.hpp"

#include <net/network_manager.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

static inline util::time::system_time_point suspendedAt;
static inline bool isSuspended = false;

#ifdef GEODE_IS_ANDROID
void GlobedAppDelegate::applicationDidEnterBackground() {
    NetworkManager::get().suspend();
    suspendedAt = util::time::systemNow();
    isSuspended = true;

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    auto& nm = NetworkManager::get();
    nm.resume();

    if (isSuspended) {
        // to avoid firing twice
        isSuspended = false;
        auto passed = util::time::systemNow() - suspendedAt;

        if (passed > util::time::seconds(60) && nm.established()) {
            NetworkManager::get().disconnectWithMessage("connection lost, application was in the background for too long");
        }
    }

    AppDelegate::applicationWillEnterForeground();
}
#endif // GEODE_IS_ANDROID

#ifdef GEODE_IS_MACOS
constexpr ptrdiff_t INLINED_START = 0x38127d;
constexpr ptrdiff_t REAL_LOADING_FINISHED = 0x381310;

void GlobedAppDelegate::loadingIsFinished() {
    AppDelegate::loadingIsFinished();

    // LoadingLayer::loadingFinished is inlined, so we hook a function that gets called before that (AppDelegate::loadingIsFinished),
    // nop out the inlined code and replace it with a call to the real loadingFinished
    // because fuck you that's why
    static auto* patch = util::lowlevel::call(INLINED_START, REAL_LOADING_FINISHED);
    if (!patch->isEnabled()) {
        (void) patch->enable().unwrap();
    }

    // nop out the leftovers
    static auto* nopPatch = util::lowlevel::nop(INLINED_START + patch->getBytes().size(), 3 + 4 + 5 + 3 + 3 + 5);
    if (!nopPatch->isEnabled()) {
        (void) nopPatch->enable().unwrap();
    }
}
#endif // GEODE_IS_MACOS