#include "Badges.hpp"

#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

static CCSprite* createAny(const char* name) {
    auto sprite = CCSprite::create(name);
    if (!sprite) {
        sprite = CCSprite::createWithSpriteFrameName(name);
    }

    return sprite;
}

CCSprite* createBadge(const char* spriteName) {
    auto name = fmt::format("{}"_spr, spriteName);

    auto sprite = createAny(name.c_str());

    if (!sprite) {
        log::warn("Invalid badge icon used: {}", name);
        sprite = createAny("role-unknown.png"_spr);
    }

    if (!sprite) {
        // still nothing??
        sprite = createAny("GJ_likesIcon_001.png");
    }

    if (!sprite) {
        return nullptr;
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