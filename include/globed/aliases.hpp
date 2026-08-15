#pragma once

#include <Geode/Result.hpp>
#include <Geode/utils/ZStringView.hpp>
#include <Geode/utils/function.hpp>
#include <cocos2d.h>

/// Aliases for omitting `geode::` and `cocos2d::` in code

namespace geode {

template <typename T>
class Ref;
template <typename T>
class WeakRef;

class Label;
class Button;
class NineSlice;

namespace cocos {
    class CCMenuItemExt;
}

namespace log {}

}

namespace cocos2d::extension {
    class CCScale9Sprite;
}

namespace globed {

template <typename T = void, typename E = std::string>
using Result = geode::Result<T, E>;

using geode::Ok;
using geode::Err;
using geode::ZStringView;
using geode::Function;
using geode::FunctionRef;

using geode::Ref;
using geode::WeakRef;
using geode::Label;
using geode::Button;
using geode::NineSlice;
using geode::cocos::CCMenuItemExt;
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
