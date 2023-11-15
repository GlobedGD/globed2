#pragma once

#include <defs.hpp>

#ifndef GLOBED_ROOT_NO_GEODE

#include <functional>

namespace util::ui {
    void addBackground(cocos2d::CCNode* layer);
    void addBackButton(cocos2d::CCNode* parent, cocos2d::CCMenu* menu, std::function<void()> callback);
    void navigateBack();
}

#endif // GLOBED_ROOT_NO_GEODE