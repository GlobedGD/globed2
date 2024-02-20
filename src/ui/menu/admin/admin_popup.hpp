#pragma once
#include <defs.hpp>

class AdminPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;

    static AdminPopup* create();

private:
    geode::InputNode *messageInput;

    void onClose(cocos2d::CCObject*) override;

    bool setup() override;
};
