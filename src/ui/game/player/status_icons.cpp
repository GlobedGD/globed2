#include "status_icons.hpp"

#include <managers/settings.hpp>

using namespace geode::prelude;

bool PlayerStatusIcons::init(unsigned char opacity) {
    if (!CCNode::init()) return false;

    this->opacity = opacity;

    this->updateStatus(false, false, false, false, 0.f);
    this->schedule(schedule_selector(PlayerStatusIcons::updateLoudnessIcon), 0.25f);

    return true;
}

void PlayerStatusIcons::updateLoudnessIcon(float dt) {
    Loudness lcat = this->loudnessToCategory(lastLoudness * 2.f);
    if (lcat != wasLoudness) {
        wasLoudness = lcat;
        this->updateStatus(wasPaused, wasPracticing, wasSpeaking, wasEditing, lastLoudness, true);
    }
}

void PlayerStatusIcons::updateStatus(bool paused, bool practicing, bool speaking, bool editing, float loudness, bool force) {
    lastLoudness = loudness;

    if (!force && wasPaused == paused && wasPracticing == practicing && wasSpeaking == speaking && wasEditing == editing) return;
    wasPaused = paused;
    wasPracticing = practicing;
    wasSpeaking = speaking;
    wasEditing = editing;

    if (!wasPaused && !wasPracticing && !wasSpeaking && !wasEditing) {
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
            .opacity(opacity)
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
            .opacity(opacity)
            .zOrder(1)
            .scale(0.8f)
            .id("icon-practice"_spr)
            .parent(iconWrapper)
            .collect();

        width += practiceSpr->getScaledContentSize().width;
        count++;
    }

    if (wasSpeaking) {
        std::string sprite;
        switch (wasLoudness) {
            case Loudness::Low: sprite = "speaker-icon.png"_spr; break;
            case Loudness::Medium: sprite = "speaker-icon-yellow.png"_spr; break;
            case Loudness::High: sprite = "speaker-icon-red.png"_spr; break;
        }

        auto speakSpr = Build<CCSprite>::createSpriteName(sprite.c_str())
            .opacity(opacity)
            .zOrder(1)
            .scale(0.85f)
            .id("icon-speaking"_spr)
            .parent(iconWrapper)
            .collect();

        width += speakSpr->getScaledContentSize().width;
        count++;
    }

    if (wasEditing) {
        auto editSpr = Build<CCSprite>::createSpriteName("GJ_hammerIcon_001.png")
            .opacity(opacity)
            .zOrder(1)
            .scale(0.8f)
            .id("icon-editing"_spr)
            .parent(iconWrapper)
            .collect();

        width += editSpr->getScaledContentSize().width;
        count++;
    }

    width += static_cast<RowLayout*>(iconWrapper->getLayout())->getGap() * (count - 1);

    auto cc9s = Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize({ width * 3.f, 40.f * 3.f })
        .scale(1.f / 3.f)
        .opacity(opacity / 3)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(cc9s->getScaledContentSize());
    iconWrapper->setContentSize(cc9s->getScaledContentSize());
    iconWrapper->setPositionY(cc9s->getScaledContentSize().height / 2);
    iconWrapper->updateLayout();
}

PlayerStatusIcons::Loudness PlayerStatusIcons::loudnessToCategory(float loudness) {
    // TODO
    return Loudness::Low;
    // if (loudness < 0.5f) {
    //     return Loudness::Low;
    // } else if (loudness < 0.75f) {
    //     return Loudness::Medium;
    // } else {
    //     return Loudness::High;
    // }
}

PlayerStatusIcons* PlayerStatusIcons::create(unsigned char opacity) {
    auto ret = new PlayerStatusIcons;
    if (ret->init(opacity)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
