#include "progress_icon.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool PlayerProgressIcon::init() {
    if (!CCNode::init()) return false;

    this->updateIcons(PlayerAccountData::DEFAULT_DATA.icons);

    return true;
}

void PlayerProgressIcon::updateIcons(const PlayerIconData& data) {
    if (line) line->removeFromParent();
    if (practiceSprite) practiceSprite->removeFromParent();
    if (playerIcon) playerIcon->removeFromParent();

    auto& settings = GlobedSettings::get();
    auto gm = GameManager::get();
    auto color1 = gm->colorForIdx(data.color1);
    auto color2 = gm->colorForIdx(data.color2);

    Build<CCLayerColor>::create(ccc4(color1.r, color1.g, color1.b, 255), 2.f, 6.f)
        .pos(0.f, 5.f)
        .parent(this)
        .store(line);
        
    Build<CCSprite>::createSpriteName("checkpoint_01_001.png")
        .pos(1.2f, 8.f)
        .scale(.3f)
        .parent(this)
        .visible(false)
        .store(practiceSprite);

    Build<GlobedSimplePlayer>::create(data)
        .opacity(forceOnTop ? 255 : static_cast<uint8_t>(settings.levelUi.progressOpacity * 255))
        .scale(0.5f)
        .anchorPoint(0.5f, 0.5f)
        .pos(0.f, -10.f)
        .parent(this)
        .store(playerIcon);
}

void PlayerProgressIcon::updatePosition(float progress, bool isPracticing) {
    auto parent = this->getParent()->getParent();
    if (!parent) return;

    CCSize pbSize = parent->getScaledContentSize();
    float prOffset = (pbSize.width - 2.f) * progress;

    bool lineIsVisible = progress > 0.01f && progress < 0.99f;

    bool practiceSpriteVisible = lineIsVisible && isPracticing;
    this->toggleLine(lineIsVisible);
    this->togglePracticeSprite(practiceSpriteVisible);

    if (forceOnTop) {
        this->setZOrder(100000);
    } else {
        this->setZOrder((int)(progress * 10000)); // straight from globed1
    }
    this->setPositionX(prOffset);
}

void PlayerProgressIcon::toggleLine(bool enabled) {
    line->setVisible(enabled);
}

void PlayerProgressIcon::togglePracticeSprite(bool enabled) {
    practiceSprite->setVisible(enabled);
}

void PlayerProgressIcon::setForceOnTop(bool state) {
    this->forceOnTop = state;
    auto& settings = GlobedSettings::get();
    this->playerIcon->setOpacity(forceOnTop ? 255 : static_cast<uint8_t>(settings.levelUi.progressOpacity * 255));
}

PlayerProgressIcon* PlayerProgressIcon::create() {
    auto ret = new PlayerProgressIcon;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
