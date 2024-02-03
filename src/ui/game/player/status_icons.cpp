#include "status_icons.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool PlayerStatusIcons::init() {
    if (!CCNode::init()) return false;

    this->updateStatus(false, false);

    return true;
}

void PlayerStatusIcons::updateStatus(bool paused, bool practicing) {
    if (wasPaused == paused && wasPracticing == practicing) return;
    wasPaused = paused;
    wasPracticing = practicing;

    if (!wasPaused && !wasPracticing) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);
    this->removeAllChildren();

    float width = 25.f;

    iconWrapper = Build<CCNode>::create()
        .anchorPoint(0.f, 0.5f)
        .layout(RowLayout::create()
                    ->setGap(6.5f))
        .parent(this);

    if (wasPaused) {
        auto pauseSpr = Build<CCSprite>::createSpriteName("GJ_pauseBtn_clean_001.png")
            .zOrder(1)
            .scale(0.8f)
            .id("icon-paused"_spr)
            .parent(iconWrapper)
            .collect();

        width += pauseSpr->getScaledContentSize().width;
    }

    if (wasPracticing) {
        auto practiceSpr = Build<CCSprite>::createSpriteName("checkpoint_01_001.png")
            .zOrder(1)
            .scale(0.8f)
            .id("icon-practice"_spr)
            .parent(iconWrapper)
            .collect();

        width += practiceSpr->getScaledContentSize().width;
    }

    if (wasPaused && wasPracticing) {
        width += static_cast<RowLayout*>(iconWrapper->getLayout())->getGap();
    }

    auto cc9s = Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize({ width * 3.f, 40.f * 3.f })
        .scale(1.f / 3.f)
        .opacity(80)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(cc9s->getScaledContentSize());
    iconWrapper->setContentSize(cc9s->getScaledContentSize());
    iconWrapper->setPositionY(cc9s->getScaledContentSize().height / 2);
    iconWrapper->updateLayout();
}

PlayerStatusIcons* PlayerStatusIcons::create() {
    auto ret = new PlayerStatusIcons;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}