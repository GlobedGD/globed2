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

    // LoadingLayer::loadingFinished is inlined, so we hook a function that gets
    // called before that (AppDelegate::loadingIsFinished) and patch the inlined code to call the original function

    // mov rdi, rbx (rbx is LoadingLayer*)
    static auto* movPatch = util::lowlevel::patch(INLINED_START, {0x48, 0x89, 0xdf});
    if (!movPatch->isEnabled()) {
        (void) movPatch->enable();
    }

    const ptrdiff_t callStart = INLINED_START + movPatch->getBytes().size();

    // add a call to real loadingFinished
    static auto* patch = util::lowlevel::call(callStart, REAL_LOADING_FINISHED);
    if (!patch->isEnabled()) {
        (void) patch->enable();
    }

    const ptrdiff_t nopStart = callStart + patch->getBytes().size();

    // nop out the leftovers
    static auto* nopPatch = util::lowlevel::nop(nopStart, 4 + 5 + 3 + 3 + 5);
    if (!nopPatch->isEnabled()) {
        (void) nopPatch->enable();
    }
}
#endif // GEODE_IS_MACOS