#pragma once

#include <Geode/Result.hpp>
#include <Geode/utils/function.hpp>
#include <Geode/utils/ZStringView.hpp>
#include <cocos2d.h>
#include <cocos-ext.h>

/// Aliases for omitting `geode::` and `cocos2d::` in code

namespace globed {

template <typename T = void, typename E = std::string>
using Result = geode::Result<T, E>;

using geode::Ok;
using geode::Err;
using geode::Ref;
using geode::Label;
using geode::ZStringView;
using geode::Function;
using geode::FunctionRef;
using geode::Button;
using cocos2d::CCPoint;
using cocos2d::CCSize;
using cocos2d::CCNode;
using cocos2d::CCObject;
using cocos2d::CCArray;
using cocos2d::CCSprite;
using cocos2d::CCLabelBMFont;
using cocos2d::CCLayer;
using cocos2d::CCLayerColor;
using cocos2d::CCMenu;
using cocos2d::ccColor3B;
using cocos2d::ccColor4B;
using cocos2d::ccColor4F;
using cocos2d::extension::CCScale9Sprite;

namespace log = geode::log;

}
