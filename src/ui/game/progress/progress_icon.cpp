#include "progress_icon.hpp"

using namespace geode::prelude;

bool PlayerProgressIcon::init(float maxLevelX) {
    if (!CCNode::init()) return false;
    maxX = maxLevelX;

    this->updateIcons(PlayerAccountData::DEFAULT_DATA.icons);

    return true;
}

void PlayerProgressIcon::updateIcons(const PlayerIconData& data) {
    if (line) line->removeFromParent();
    if (playerIcon) playerIcon->removeFromParent();

    auto gm = GameManager::get();
    auto color1 = gm->colorForIdx(data.color1);
    auto color2 = gm->colorForIdx(data.color2);

    // TODO just put this behind the progress bar for gods sake
    Build<CCLayerColor>::create(ccc4(color1.r, color1.g, color1.b, 255), 2.f, 8.f)
        .pos(0.f, 3.f)
        .parent(this)
        .store(line);

    Build<SimplePlayer>::create(data.cube)
        .color(color1)
        .secondColor(color2)
        .scale(0.5f)
        .pos(0.f, -10.f)
        .parent(this)
        .store(playerIcon);

    if (data.glowColor != -1) {
        playerIcon->setGlowOutline(gm->colorForIdx(data.glowColor));
    }
}

void PlayerProgressIcon::updateMaxLevelX(float maxLevelX) {
    maxX = maxLevelX;
}

void PlayerProgressIcon::updatePosition(float xPosition) {
    auto parent = this->getParent()->getParent();
    if (!parent) return;

    float progress = this->calculateRatio(xPosition, maxX);
    CCSize pbSize = parent->getScaledContentSize();
    float prOffset = (pbSize.width - 2.f) * progress;

    this->toggleLine(progress > 0.01f && progress < 0.99f);
    this->setZOrder((int)(progress * 10000)); // straight from globed1
    this->setPositionX(prOffset);
}

void PlayerProgressIcon::toggleLine(bool enabled) {
    line->setVisible(enabled);
}

PlayerProgressIcon* PlayerProgressIcon::create(float maxLevelX) {
    auto ret = new PlayerProgressIcon;
    if (ret->init(maxLevelX)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
