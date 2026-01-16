#pragma once

#include <globed/util/CStr.hpp>

#include <Geode/Geode.hpp>

namespace globed {

static constexpr cocos2d::CCSize BADGE_SIZE{16.f, 16.f};

cocos2d::CCSprite *createBadge(CStr spriteName);
cocos2d::CCSprite *createBadge(uint8_t roleId);
cocos2d::CCSprite *createMyBadge();

} // namespace globed