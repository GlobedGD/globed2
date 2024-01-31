#pragma once

#include <defs.hpp>
#include <functional>

namespace util::ui {
    void switchToScene(cocos2d::CCLayer* layer);
    void prepareLayer(cocos2d::CCLayer* layer);
    void addBackground(cocos2d::CCNode* layer);
    void addBackButton(cocos2d::CCMenu* menu, std::function<void()> callback);
    void navigateBack();
}
