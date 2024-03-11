#include <defs/geode.hpp>
#include <Geode/modify/CCSprite.hpp>

using namespace geode::prelude;

class $modify(CCSprite) {
    $override
    bool initWithSpriteFrame(CCSpriteFrame* frame) {
        if (!frame) {
            log::warn("missing texture: replacing with placeholder");
            auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
            frame = cache->spriteFrameByName("GJ_reportBtn_001.png");
        }

        return CCSprite::initWithSpriteFrame(frame);
    }

    // needed because there is a nullptr check in create too
    $override
    static CCSprite* createWithSpriteFrame(CCSpriteFrame* frame) {
        if (!frame) {
            log::warn("missing texture: replacing with placeholder");
            auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
            frame = cache->spriteFrameByName("GJ_reportBtn_001.png");
        }

        return CCSprite::createWithSpriteFrame(frame);
    }
};
