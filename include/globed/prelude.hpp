#pragma once

#include "config.hpp"
#include "util/singleton.hpp"
#include "util/CStr.hpp"
#include "util/color.hpp"
#include "util/ConstexprString.hpp"
#include "util/assert.hpp"
#include "util/lazy.hpp"
#include "util/Random.hpp"
#include "core/SettingsManager.hpp"
#include "core/PopupManager.hpp"
#include "core/Core.hpp"
#include "core/Constants.hpp"
#include "core/actions.hpp"
#include <Geode/Result.hpp>

namespace globed {

template <typename T = void, typename E = std::string>
using Result = geode::Result<T, E>;

using geode::Ok;
using geode::Err;
using geode::Ref;
using cocos2d::CCPoint;
using cocos2d::CCSize;
using cocos2d::CCNode;
using cocos2d::CCObject;
using cocos2d::CCArray;
using cocos2d::CCSprite;
using cocos2d::CCLabelBMFont;
using cocos2d::CCLayer;
using cocos2d::CCMenu;
using cocos2d::extension::CCScale9Sprite;

namespace log = geode::log;

}

template <typename T>
T* get() {
    return globed::cachedSingleton<T>();
}
