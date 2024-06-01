#include "appdelegate.hpp"

#include <net/manager.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_ANDROID
void GlobedAppDelegate::applicationDidEnterBackground() {
    NetworkManager::get().suspend();

    AppDelegate::applicationDidEnterBackground();
}

void GlobedAppDelegate::applicationWillEnterForeground() {
    auto& nm = NetworkManager::get();
    nm.resume();

    AppDelegate::applicationWillEnterForeground();
}
#endif // GEODE_IS_ANDROID

#if defined(GEODE_IS_MACOS)
static_assert(GEODE_COMP_GD_VERSION == 22000, "update these hardcoded patches thank you");

// TODO: update for 2.206
constexpr ptrdiff_t INLINED_START = 0x38127d; // start of the inlined LoadingLayer::loadingFinished
constexpr ptrdiff_t REAL_LOADING_FINISHED = 0x381310; // real address of the func (it's just a bit later)

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