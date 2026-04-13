#include "PingOverlay.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool PingOverlay::init() {
    CCNode::init();

    auto winSize = CCDirector::get()->getWinSize();

    m_pingLabel = Build<Label>::create("", "bigFont.fnt")
        .parent(this)
        .id("ping-label"_spr);

    // show version label for any version that isn't a release
    auto version = Mod::get()->getVersion();
    if (version.getTag()) {
        m_versionLabel = Build<Label>::create(version.toVString(), "bigFont.fnt")
            .parent(this)
            .id("version-label"_spr);
    }

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
        ->setGap(2.f)
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
    float baseOp = globed::setting<float>("core.overlay.opacity");
    float pingOp = baseOp;

    // if there is packet loss, make the overlay slightly more visible
    if (m_prevLoss >= 0.01f) {
        float mult = 1.5f + m_prevLoss;
        pingOp = std::clamp(pingOp * mult, 0.0f, 1.0f);
    }

    m_pingLabel->setOpacity(static_cast<uint8_t>(pingOp * 255.f));

    if (m_versionLabel) {
        m_versionLabel->setOpacity(static_cast<uint8_t>(baseOp * 255.f));
    }
}

static ccColor3B colorForPingAndLoss(uint32_t ping, float loss) {
    // color based on loss, don't include ping
    if (loss < 0.05f) { // < 5%
        return ccColor3B{ 255, 255, 255 };
    } else if (loss < 0.15f) { // < 15%
        return ccColor3B{ 255, 255, 0 };
    } else if (loss < 0.30f) { // < 30%
        return ccColor3B{ 255, 128, 0 };
    } else { // >= 30%
        return ccColor3B{255, 50, 50};
    }
}

void PingOverlay::updatePing() {
    if (!m_enabled) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);

    auto& nm = NetworkManagerImpl::get();
    bool connected = nm.isGameConnected();
    auto ping = nm.getGamePing().millis();
    auto loss = nm.getGameLoss();

    if (!connected) {
        m_pingLabel->setString("? ms");
    } else if (loss <= 0.01f) {
        m_pingLabel->setString(fmt::format("{} ms", ping));
    } else {
        m_pingLabel->setString(fmt::format("{} ms ({:.1f}% loss)", ping, loss * 100.f));
    }

    m_pingLabel->setColor(colorForPingAndLoss(ping, loss));
    this->updateLayout();

    if (m_prevLoss != loss) {
        m_prevLoss = loss;
        this->updateOpacity();
    }
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