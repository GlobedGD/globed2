#pragma once

#include "config.hpp"
#include "core/Constants.hpp"
#include "core/Core.hpp"
#include "core/PopupManager.hpp"
#include "core/SettingsManager.hpp"
#include "core/actions.hpp"
#include "util/CStr.hpp"
#include "util/ConstexprString.hpp"
#include "util/assert.hpp"
#include "util/color.hpp"
#include "util/lazy.hpp"
#include "util/singleton.hpp"
#include <Geode/Result.hpp>
#include <std23/function_ref.h>
#include <std23/move_only_function.h>

namespace globed {

template <typename T = void, typename E = std::string> using Result = geode::Result<T, E>;

using cocos2d::CCArray;
using cocos2d::ccColor3B;
using cocos2d::ccColor4B;
using cocos2d::ccColor4F;
using cocos2d::CCLabelBMFont;
using cocos2d::CCLayer;
using cocos2d::CCMenu;
using cocos2d::CCNode;
using cocos2d::CCObject;
using cocos2d::CCPoint;
using cocos2d::CCSize;
using cocos2d::CCSprite;
using cocos2d::extension::CCScale9Sprite;
using geode::Err;
using geode::Ok;
using geode::Ref;

namespace log = geode::log;

template <typename T> T *get()
{
    return globed::cachedSingleton<T>();
}

} // namespace globed

#define GLOBED_NOCOPY(cls)                                                                                             \
    cls(const cls &) = delete;                                                                                         \
    cls &operator=(const cls &) = delete

#define GLOBED_NOMOVE(cls)                                                                                             \
    cls(cls &&) = delete;                                                                                              \
    cls &operator=(cls &&) = delete
