#pragma once

#include <Geode/Geode.hpp>
#include <core/net/NetworkManagerImpl.hpp>

namespace globed {

static constexpr cocos2d::CCSize BADGE_SIZE { 48.f, 48.f };

cocos2d::CCSprite* createBadge(const char* spriteName);
cocos2d::CCSprite* createBadge(uint8_t roleId);
cocos2d::CCSprite* createMyBadge();

}