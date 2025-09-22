#include <globed/core/game/PlayerStatusIcons.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool PlayerStatusIcons::init(unsigned char opacity) {
    if (!CCNode::init()) return false;

    m_opacity = opacity;

    this->updateStatus(PlayerStatusFlags{}, true);

    return true;
}

void PlayerStatusIcons::updateStatus(const PlayerStatusFlags& flags, bool force) {
    if (!force && m_flags == flags) {
        return;
    }

    m_flags = flags;

    this->removeAllChildren();

    if (!flags.paused && !flags.practicing && !flags.speaking && !flags.editing && !flags.speakingMuted) {
        return;
    }

    m_iconWrapper = Build<CCNode>::create()
        .anchorPoint(0.f, 0.5f)
        .layout(RowLayout::create()->setGap(6.5f)->setAutoScale(false))
        .parent(this);

    CCSize iconSize{24.f, 24.f};

    float width = 20.f;
    size_t count = 0;

    auto addButton = [&](CCSprite* spr, const std::string& id) {
        auto btn = Build(spr)
            .opacity(m_opacity)
            .zOrder(1)
            .with([&](CCSprite* spr) {
                cue::rescaleToMatch(spr, iconSize);
            })
            .id(id)
            .parent(m_iconWrapper)
            .collect();

        width += btn->getScaledContentWidth();
        count++;
        return btn;
    };

    if (flags.paused) {
        addButton(CCSprite::createWithSpriteFrameName("GJ_pauseBtn_clean_001.png"), "icon-paused");
    }

    if (flags.practicing) {
        addButton(CCSprite::createWithSpriteFrameName("checkpoint_01_001.png"), "icon-practice");
    }

    if (flags.speaking) {
        const char* sprite;

        if (flags.speakingMuted) {
            sprite = "speaking-icon-mute.png"_spr;
        } else {
            sprite = "speaker-icon.png"_spr;
        }

        addButton(CCSprite::createWithSpriteFrameName(sprite), "icon-speaking");
    }

    if (flags.editing) {
        addButton(CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png"), "icon-editing");
    }

    width += static_cast<RowLayout*>(m_iconWrapper->getLayout())->getGap() * (count - 1);

    auto cc9s = Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize({ width * 3.f, 40.f * 3.f })
        .scale(1.f / 3.f)
        .opacity(m_opacity / 3)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(cc9s->getScaledContentSize());
    m_iconWrapper->setContentSize(cc9s->getScaledContentSize());
    m_iconWrapper->setPositionY(cc9s->getScaledContentSize().height / 2);
    m_iconWrapper->updateLayout();
}

PlayerStatusIcons* PlayerStatusIcons::create(unsigned char opacity) {
    auto ret = new PlayerStatusIcons();
    if (ret->init(opacity)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}