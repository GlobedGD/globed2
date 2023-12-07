#pragma once

#include <defs.hpp>
#include <functional>

namespace util::ui {
    void addBackground(cocos2d::CCNode* layer);
    void addBackButton(cocos2d::CCMenu* menu, std::function<void()> callback);
    void navigateBack();
}
