#pragma once

#include <globed/util/CStr.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <Geode/Geode.hpp>

namespace globed {

static constexpr cocos2d::CCSize BADGE_SIZE { 32.f, 32.f };

cocos2d::CCSprite* createBadge(CStr spriteName);
cocos2d::CCSprite* createBadge(uint8_t roleId);
cocos2d::CCSprite* createMyBadge();

}