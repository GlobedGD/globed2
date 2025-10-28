#include "EmoteBubble.hpp"

#include <UIBuilder.hpp>
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
        .anchorPoint(0, 0)
        .scale(0.3f)
        .parent(this);

    m_emoteSpr->setPosition((m_bubbleSpr->getContentSize() / 2.f) + CCPoint{0, 15.f});
    m_bubbleSpr->addChild(m_emoteSpr);

    this->customToggleVis(false);

    return true;
}

void EmoteBubble::playEmote(uint32_t emoteId) {
    auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();

    std::string newFrameName = fmt::format("emote_{}.png"_spr, emoteId).c_str();

    if (cache->spriteFrameByName(newFrameName.c_str())) {
        auto newFrame = cache->spriteFrameByName(newFrameName.c_str());
        m_emoteSpr->setDisplayFrame(newFrame);
        cue::rescaleToMatch(m_emoteSpr, {50.f, 50.f});

        this->customToggleVis(true);

        m_bubbleSpr->setScale(0.2f);
        m_bubbleSpr->runAction(
            CCSequence::create(
                CCEaseExponentialOut::create(CCScaleTo::create(0.4f, 0.5f)),
                CCDelayTime::create(1.7f),
                CCEaseExponentialIn::create(CCScaleTo::create(0.4f, 0.2f)),
                nullptr
            )
        );

        m_emoteSpr->runAction(
        CCSpawn::create(
                CCEaseExponentialOut::create(CCMoveBy::create(0.6f, {0, 10.f})),
                CCEaseBounceOut::create(CCMoveBy::create(0.75f, {0, -10.f})),
                0
            )
        );

        auto fmod = FMODAudioEngine::sharedEngine();
        std::string path = fmt::format("emote_sfx_{}.ogg"_spr, emoteId).c_str();
        fmod->playEffect(path, 1.f + rng()->random(-0.05f, 0.05f), 1.f, 0.75f);

        this->runAction(
            CCSequence::create(
                CCDelayTime::create(2.5f),
                CallFuncExt::create([this]() { this->customToggleVis(false); }),
                nullptr
            )
        );
    }
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