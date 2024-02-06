#include "status_icons.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool PlayerStatusIcons::init() {
    if (!CCNode::init()) return false;

    this->updateStatus(false, false, false);

    return true;
}

void PlayerStatusIcons::updateStatus(bool paused, bool practicing, bool speaking) {
    if (wasPaused == paused && wasPracticing == practicing && wasSpeaking == speaking) return;
    wasPaused = paused;
    wasPracticing = practicing;
    wasSpeaking = speaking;

    if (!wasPaused && !wasPracticing && !wasSpeaking) {
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

    size_t count = 0;

    if (wasPaused) {
        auto pauseSpr = Build<CCSprite>::createSpriteName("GJ_pauseBtn_clean_001.png")
            .zOrder(1)
            .scale(0.8f)
            .id("icon-paused"_spr)
            .parent(iconWrapper)
            .collect();

        width += pauseSpr->getScaledContentSize().width;
        count++;
    }

    if (wasPracticing) {
        auto practiceSpr = Build<CCSprite>::createSpriteName("checkpoint_01_001.png")
            .zOrder(1)
            .scale(0.8f)
            .id("icon-practice"_spr)
            .parent(iconWrapper)
            .collect();

        width += practiceSpr->getScaledContentSize().width;
        count++;
    }

    if (wasSpeaking) {
        auto speakSpr = Build<CCSprite>::createSpriteName("speaker-icon.png"_spr)
            .zOrder(1)
            .scale(0.8f)
            .id("icon-speaking"_spr)
            .parent(iconWrapper)
            .collect();

        width += speakSpr->getScaledContentSize().width;
        count++;
    }

    width += static_cast<RowLayout*>(iconWrapper->getLayout())->getGap() * (count - 1);

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