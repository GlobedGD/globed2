#include <globed/core/game/ProgressIcon.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/singleton.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool ProgressIcon::init() {
    if (!CCNode::init()) return false;

    this->updateIcons(cue::Icons{});

    return true;
}

void ProgressIcon::updateIcons(const cue::Icons& data) {
    if (m_line) {
        m_line->removeFromParent();
    }

    if (m_icon) {
        m_icon->removeFromParent();
    }

    auto col1 = cachedSingleton<GameManager>()->colorForIdx(data.color1);

    m_line = Build<CCLayerColor>::create(ccColor4B{col1.r, col1.g, col1.b, 255}, 2.f, 6.f)
        .pos(0.f, 5.f)
        .parent(this);

    float progressOpacity = globed::setting<float>("core.level.progress-opacity");

    m_icon = Build(cue::PlayerIcon::create(data))
        .scale(0.5f)
        .anchorPoint(0.5f, 0.5f)
        .pos(0.f, -10.f)
        .parent(this);

    this->recalcOpacity();
}

void ProgressIcon::updatePosition(float progress, bool isPracticing) {
    auto parent = this->getParent()->getParent();
    if (!parent) return;

    CCSize pbSize = parent->getScaledContentSize();
    float prOffset = (pbSize.width - 2.f) * progress;

    bool lineIsVisible = progress > 0.01f && progress < 0.99f;

    bool practiceSpriteVisible = lineIsVisible && isPracticing;
    this->toggleLine(lineIsVisible);
    this->togglePracticeSprite(practiceSpriteVisible);

    if (m_forceOnTop) {
        this->setZOrder(100000);
    } else {
        this->setZOrder((int)(progress * 10000)); // straight from globed1
    }

    this->setPositionX(prOffset);
}

void ProgressIcon::toggleLine(bool enabled) {
    m_line->setVisible(enabled);
}

void ProgressIcon::togglePracticeSprite(bool enabled) {

}

void ProgressIcon::setForceOnTop(bool state) {
    m_forceOnTop = state;
    this->recalcOpacity();
}

void ProgressIcon::recalcOpacity() {
    float progressOpacity = globed::setting<float>("core.level.progress-opacity");
    m_icon->setOpacity(m_forceOnTop ? 255 : static_cast<uint8_t>(progressOpacity * 255));
}

ProgressIcon* ProgressIcon::create() {
    auto ret = new ProgressIcon;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}


}