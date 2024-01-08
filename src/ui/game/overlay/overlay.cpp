#include "overlay.hpp"

#include <UIBuilder.hpp>

#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedOverlay::init() {
    if (!CCNode::init()) return false;

    auto& gs = GlobedSettings::get();
    auto& settings = gs.overlay;

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .opacity(settings.overlayOpacity)
        .store(pingLabel)
        .parent(this)
        .id("ping-label"_spr);

    return true;
}

void GlobedOverlay::updatePing(uint32_t ms) {
    auto fmted = fmt::format("{} ms", ms);
    pingLabel->setString(fmted.c_str());
}

GlobedOverlay* GlobedOverlay::create() {
    auto ret = new GlobedOverlay;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}