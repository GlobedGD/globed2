#include "overlay.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedOverlay::init() {
    if (!CCNode::init()) return false;

    auto& gs = GlobedSettings::get();
    auto& settings = gs.overlay;

    if (!settings.enabled) {
        return true;
    }

    this->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisLineAlignment(AxisAlignment::End)
            ->setGap(3.f)
    );

    Build<CCLabelBMFont>::create("", "bigFont.fnt")
        .opacity(static_cast<uint8_t>(settings.opacity * 255))
        .store(pingLabel)
        .parent(this)
        .id("ping-label"_spr);

#ifdef GLOBED_DEBUG
    std::string versionStr = Mod::get()->getVersion().toVString();
    Build<CCLabelBMFont>::create(versionStr.c_str(), "bigFont.fnt")
        .opacity(static_cast<uint8_t>(settings.opacity * 255))
        .store(versionLabel)
        .parent(this)
        .id("version-label"_spr);
#endif

    this->setContentHeight(CCDirector::get()->getWinSize().height);
    this->updateLayout();

    return true;
}

void GlobedOverlay::updatePing(uint32_t ms) {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    auto fmted = fmt::format("{} ms", ms);
    pingLabel->setString(fmted.c_str());
    this->setVisible(true);
    this->updateLayout();
}

void GlobedOverlay::updateWithDisconnected() {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    if (settings.overlay.hideConditionally) {
        this->setVisible(false);
        return;
    }

    pingLabel->setString("Not connected");
    this->updateLayout();
}

void GlobedOverlay::updateWithEditor() {
    auto& settings = GlobedSettings::get();
    if (!settings.overlay.enabled) return;

    if (settings.overlay.hideConditionally) {
        this->setVisible(false);
        return;
    }

    pingLabel->setString("N/A (Local level)");
    this->updateLayout();
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