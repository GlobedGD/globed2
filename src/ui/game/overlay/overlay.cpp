#include "overlay.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedOverlay::init() {
    if (!CCNode::init()) return false;

    auto& gs = GlobedSettings::get();
    auto& settings = gs.overlay;

    if (settings.enabled) {
        Build<CCLabelBMFont>::create("", "bigFont.fnt")
            .opacity(static_cast<uint8_t>(settings.opacity * 255))
            .anchorPoint(0.f, 0.f)
            .store(pingLabel)
            .parent(this)
            .id("ping-label"_spr);

        this->setContentSize(pingLabel->getContentSize());
    }

    return true;
}

void GlobedOverlay::updatePing(uint32_t ms) {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    auto fmted = fmt::format("{} ms", ms);
    pingLabel->setString(fmted.c_str());
    this->setContentSize(pingLabel->getContentSize());
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
    this->setContentSize(pingLabel->getContentSize());
}

void GlobedOverlay::updateWithEditor() {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    if (settings.overlay.hideConditionally) {
        this->setVisible(false);
        return;
    }

    pingLabel->setString("N/A (Local level)");
    this->setContentSize(pingLabel->getContentSize());
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