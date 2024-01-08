#include "overlay.hpp"

#include <UIBuilder.hpp>

#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedOverlay::init() {
    if (!CCNode::init()) return false;

    auto& gs = GlobedSettings::get();
    auto& settings = gs.overlay;

    if (settings.enabled) {
        Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .opacity(settings.opacity)
            .store(pingLabel)
            .parent(this)
            .id("ping-label"_spr);
    }

    return true;
}

void GlobedOverlay::updatePing(uint32_t ms) {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    auto fmted = fmt::format("{} ms", ms);
    pingLabel->setString(fmted.c_str());
    this->setVisible(true);
}

void GlobedOverlay::updateWithDisconnected() {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    if (settings.overlay.hideConditionally) {
        this->setVisible(false);
        return;
    }

    pingLabel->setString("Not connected");
}

void GlobedOverlay::updateWithEditor() {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    if (settings.overlay.hideConditionally) {
        this->setVisible(false);
        return;
    }

    pingLabel->setString("N/A (Local level)");
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