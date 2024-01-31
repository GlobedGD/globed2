#include "progress_icon.hpp"

using namespace geode::prelude;

bool PlayerProgressIcon::init(float maxLevelX) {
    if (!CCNode::init()) return false;
    maxX = maxLevelX;

    return true;
}

void PlayerProgressIcon::updatePosition(float xPosition) {
    auto parent = this->getParent();
    if (!parent) return;

    float progress = this->calculateRatio(xPosition, maxX);
    CCSize pbSize = parent->getScaledContentSize();
    float prOffset = (pbSize.width - 2.f) * progress;

    this->toggleLine(progress > 0.01f && progress < 0.99f);
    this->setZOrder((int)(progress * 10000)); // straight from globed1
    this->setPositionX(prOffset);
}

void PlayerProgressIcon::toggleLine(bool enabled) {
    // TODO
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
