#include "cclayer.hpp"

#include <hooks/play_layer.hpp>

using namespace geode::prelude;

#ifndef GEODE_IS_WINDOWS

void HookedCCLayer::onEnter() {
    // on android, it is not possible to hook PlayLayer::onEnter by patching the vtable, due to memory protection issues.
    // instead, we hook CCLayer::onEnter directly, and check if this is a PlayLayer.
    if (reinterpret_cast<void*>(PlayLayer::get()) == reinterpret_cast<void*>(this)) {
        auto* gpl = static_cast<GlobedPlayLayer*>(reinterpret_cast<PlayLayer*>(this));
        if (gpl->established()) {
            gpl->onEnterHook();
        } else {
            CCLayer::onEnter();
        }
    } else {
        CCLayer::onEnter();
    }
}

#endif // GEODE_IS_WINDOWS