#include "Badges.hpp"

#include <core/net/NetworkManagerImpl.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

static bool isValid(CCSprite* spr) {
    return spr && !spr->isUsingFallback();
}

static CCSprite* createAny(ZStringView name) {
    CCSprite* sprite = nullptr;

    // do this to bypass the silly log from geode
    auto frame = (CCSpriteFrame*) CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(name);
    if (frame) {
        sprite = CCSprite::createWithSpriteFrame(frame);
    }

    if (!isValid(sprite)) {
        sprite = CCSprite::create(name.c_str());
    }

    return isValid(sprite) ? sprite : nullptr;
}

CCSprite* createBadge(ZStringView spriteName) {
    // first try mod-prefixed sprites
    auto sprite = createAny(fmt::format("{}"_spr, spriteName));

    // then try general gd sprites
    if (!sprite) {
        sprite = createAny(spriteName);
    }

    // then fallback to unknown role icon
    if (!sprite) {
        sprite = createAny("role-unknown.png"_spr);
    }

    // still nothing, user has no resources for the mod?
    if (!sprite) {
        log::warn("Invalid badge icon used: '{}'", spriteName);
        sprite = CCSprite::createWithSpriteFrameName("GJ_likesIcon_001.png");
    }

    cue::rescaleToMatch(sprite, BADGE_SIZE);

    return sprite;
}

CCSprite* createBadge(uint8_t roleId) {
    if (auto role = NetworkManagerImpl::get().findRole(roleId)) {
        return createBadge(role->icon.c_str());
    } else {
        log::debug("role not found for ID {}", (int)roleId);
        return nullptr;
    }
}

CCSprite* createMyBadge() {
    auto& nm = NetworkManagerImpl::get();

    if (auto role = nm.getUserHighestRole()) {
        return createBadge(role->icon.c_str());
    } else {
        return nullptr;
    }
}

}