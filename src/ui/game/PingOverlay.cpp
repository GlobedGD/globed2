#include "PingOverlay.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool PingOverlay::init() {
    CCNode::init();

    auto winSize = CCDirector::get()->getWinSize();

    m_pingLabel = Build<Label>::create("", "bigFont.fnt")
        .parent(this)
        .id("ping-label"_spr);

#ifdef GLOBED_DEBUG
    m_versionLabel = Build<Label>::create(Mod::get()->getVersion().toVString(), "bigFont.fnt")
        .parent(this)
        .id("version-label"_spr);
#endif

    this->setContentHeight(winSize.height);
    this->reloadFromSettings();

    return true;
}

void PingOverlay::addToLayer(CCNode* parent) {
    Ref _self(this);

    this->removeFromParent();
    parent->addChild(this);

    auto winSize = CCDirector::get()->getWinSize();
    auto epos = globed::setting<int>("core.overlay.position");

    bool onTop = epos < 2;
    bool onRight = epos % 2 == 1;

    float oby = onTop ? winSize.height - 2.f : 2.f;
    float obx = onRight ? winSize.width - 2.f : 2.f;

    float anchorX = onRight ? 1.f : 0.f;
    float anchorY = onTop ? 1.f : 0.f;

    this->setLayout(ColumnLayout::create()
        ->setGap(3.f)
        ->setAxisReverse(!onTop)
        ->setAxisAlignment(onTop ? AxisAlignment::End : AxisAlignment::Start)
        ->setCrossAxisLineAlignment(onRight ? AxisAlignment::End : AxisAlignment::Start)
    );

    this->setPosition(obx, oby);
    this->setAnchorPoint({anchorX, anchorY});
    this->updateLayout();
}

void PingOverlay::reloadFromSettings() {
    m_enabled = globed::setting<bool>("core.overlay.enabled");
    m_conditional = !globed::setting<bool>("core.overlay.always-show");

    this->setVisible(m_enabled);
    this->updateOpacity();
}

void PingOverlay::updateOpacity() {
    auto op = static_cast<uint8_t>(globed::setting<float>("core.overlay.opacity") * 255.f);

    m_pingLabel->setOpacity(op);

    if (m_versionLabel) {
        m_versionLabel->setOpacity(op);
    }
}

void PingOverlay::updatePing(uint32_t ms) {
    if (!m_enabled) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);
    m_pingLabel->setString(fmt::format("{} ms", ms));
    this->updateLayout();
}

void PingOverlay::updateWithDisconnected() {
    if (!m_enabled || m_conditional) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);
    m_pingLabel->setString("Not connected");
    this->updateLayout();
}

void PingOverlay::updateWithEditor() {
    if (!m_enabled || m_conditional) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);
    m_pingLabel->setString("N/A (Local level)");
    this->updateLayout();
}

PingOverlay* PingOverlay::create() {
    auto ret = new PingOverlay;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}