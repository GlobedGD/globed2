#pragma once
#include <defs/geode.hpp>

class DailyPopup : public geode::Popup<> {
protected:

    static constexpr float POPUP_WIDTH = 420.f;
    static constexpr float POPUP_HEIGHT = 280.f;

    bool setup() override;

    cocos2d::extension::CCScale9Sprite* background;

    //void onReload(cocos2d::CCObject* sender);

public:
    static DailyPopup* create();
    void openLevel(CCObject*);
};