#include "EmoteBubble.hpp"
#include <globed/core/EmoteManager.hpp>

#include <UIBuilder.hpp>
#include <asp/format.hpp>
#include <cue/Util.hpp>
#include <globed/util/Random.hpp>

using namespace geode::prelude;

namespace globed {

EmoteBubble* EmoteBubble::create() {
    auto ret = new EmoteBubble();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool EmoteBubble::init() {
    if (!CCNode::init()) return false;

    m_emoteSpr = CCSprite::createWithSpriteFrameName("emote_0.png"_spr);

    m_bubbleSpr = Build<CCSprite>::createSpriteName("bubble_emote_spr.png"_spr)
        .anchorPoint(0.f, 0.f)
        .parent(this);
    cue::rescaleToMatch(m_bubbleSpr, 56.f);
    m_initialScale = m_bubbleSpr->getScale();

    m_bubbleSpr->addChild(m_emoteSpr);

    this->customToggleVis(false);

    return true;
}

void EmoteBubble::playEmote(uint32_t emoteId) {
    auto& em = EmoteManager::get();
    auto sprite = em.createEmote(emoteId);

    if (!sprite) {
        log::debug("Unknown emote playing: {}", emoteId);
        return;
    }

    cue::resetNode(m_emoteSpr);
    m_emoteSpr = sprite;
    m_bubbleSpr->addChild(m_emoteSpr);

    cue::rescaleToMatch(m_emoteSpr, 28.f / m_initialScale);
    m_emoteSpr->setPosition(m_bubbleSpr->getContentSize() * CCPoint{0.5f, 0.65f});

    this->customToggleVis(true);

    m_bubbleSpr->stopAllActions();
    m_emoteSpr->stopAllActions();
    this->stopAllActions();

    m_bubbleSpr->setScale(m_initialScale * 0.4f);
    m_bubbleSpr->runAction(
        CCSequence::create(
            CCEaseExponentialOut::create(CCScaleTo::create(0.4f, m_initialScale)),
            CCDelayTime::create(1.7f),
            CCEaseExponentialIn::create(CCScaleTo::create(0.4f, m_initialScale * 0.05f)),
            nullptr
        )
    );

    m_emoteSpr->runAction(
        CCSpawn::create(
            CCEaseExponentialOut::create(CCMoveBy::create(0.6f, {0, 10.f})),
            CCEaseBounceOut::create(CCMoveBy::create(0.75f, {0, -10.f})),
            nullptr
        )
    );

    em.playEmoteSfx(emoteId);

    this->runAction(
        CCSequence::create(
            CCDelayTime::create(2.5f),
            CallFuncExt::create([this]() { this->customToggleVis(false); }),
            nullptr
        )
    );
}

void EmoteBubble::setOpacity(uint8_t op) {
    m_bubbleSpr->setOpacity(op);
    m_emoteSpr->setOpacity(op);
}

bool EmoteBubble::isPlaying() {
    return m_bubbleSpr->isVisible();
}

void EmoteBubble::customToggleVis(bool vis) {
    m_bubbleSpr->setVisible(vis);
    m_emoteSpr->setVisible(vis);
}

}