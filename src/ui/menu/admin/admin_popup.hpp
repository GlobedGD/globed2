#pragma once
#include <defs/all.hpp>

class AdminPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 180.f;

    static AdminPopup* create();

private:
    geode::InputNode *messageInput, *userInput;

    bool setup() override;
};
