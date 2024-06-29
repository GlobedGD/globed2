#include "daily_cache.hpp"
#include "Geode/binding/GameToolbox.hpp"
#include <util/ui.hpp>

using namespace geode::prelude;

void DailyCacheManager::setStoredLevel(GJGameLevel* level) {
    storedLevel = level;
}

GJGameLevel* DailyCacheManager::getStoredLevel() {
    return storedLevel;
}

void DailyCacheManager::attachRatingSprite(int tier, CCNode* parent) {

    CCObject* obj;
    CCARRAY_FOREACH(parent->getChildren(), obj) {
        if (CCNode* node = typeinfo_cast<CCNode*>(obj)) {
            node->setVisible(false);
        }
    }

    CCSprite* spr;
    switch(tier) {
        case 1:
            spr = CCSprite::createWithSpriteFrameName("icon-epic.png"_spr);
        break;
        case 2:
            spr = CCSprite::createWithSpriteFrameName("icon-outstanding.png"_spr);
        break;
        case 0:
        default:
            spr = CCSprite::createWithSpriteFrameName("icon-featured.png"_spr);
        break;
    }
    spr->setZOrder(-1);
    spr->setPosition(parent->getScaledContentSize() / 2);
    spr->setID("globed-rating"_spr);
    parent->addChild(spr);

    if (tier == 2) {
        CCSprite* overlay = CCSprite::createWithSpriteFrameName("icon-outstanding-overlay.png"_spr);
        overlay->setPosition({parent->getScaledContentWidth() / 2, parent->getScaledContentHeight() / 2 + 2.f});
        overlay->setBlendFunc({GL_ONE, GL_ONE});
        overlay->setColor({200, 255, 255});
        overlay->setOpacity(175);
        overlay->setZOrder(1);
        parent->addChild(overlay);

        auto particle = GameToolbox::particleFromString("26a-1a1.25a0.3a16a90a62a4a0a20a20a0a16a0a0a0a0a4a2a0a0a0.341176a0a1a0a0.635294a0a1a0a0a1a0a0a0.247059a0a1a0a0.498039a0a1a0a0.16a0a0.23a0a0a0a0a0a0a0a0a2a1a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0", NULL, false);
        particle->setPosition({parent->getScaledContentWidth() / 2, parent->getScaledContentHeight() / 2 + 4.f});
        particle->setZOrder(-2);
        parent->addChild(particle);
    }
}