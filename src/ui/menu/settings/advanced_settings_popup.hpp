#pragma once
#include <defs/geode.hpp>

class AdvancedSettingsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 240.f;
    static constexpr float POPUP_HEIGHT = 200.f;

    static AdvancedSettingsPopup* create();

private:
    bool setup() override;

    void onPacketLog(cocos2d::CCObject*);
};
