#pragma once

#include <Geode/Geode.hpp>
#include <globed/prelude.hpp>

namespace globed {

static constexpr CCSize BADGE_SIZE { 16.f, 16.f };

CCSprite* createBadge(ZStringView spriteName);
CCSprite* createBadge(uint8_t roleId);
CCSprite* createMyBadge();

}