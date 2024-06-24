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

    auto winSize = CCDirector::get()->getWinSize();

    bool onTop = settings.position < 2;
    bool onRight = settings.position % 2 == 1;

    float overlayBaseY = onTop ? winSize.height - 2.f : 2.f;
    float overlayBaseX = onRight ? winSize.width - 2.f : 2.f;

    float overlayAnchorY = onTop ? 1.f : 0.f;
    float overlayAnchorX = onRight ? 1.f : 0.f;

    auto* layout = ColumnLayout::create()
        ->setAxisAlignment(AxisAlignment::Start)
        ->setGap(3.f);

    layout->setAxisReverse(!onTop);
    layout->setAxisAlignment(onTop ? AxisAlignment::End : AxisAlignment::Start);
    layout->setCrossAxisLineAlignment(onRight ? AxisAlignment::End : AxisAlignment::Start);

    this->setLayout(layout);
    this->setPosition(overlayBaseX, overlayBaseY);
    this->setAnchorPoint({overlayAnchorX, overlayAnchorY});

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