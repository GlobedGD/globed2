#pragma once
#include <defs/all.hpp>

#include <data/packets/client/admin.hpp>

class AdminLoginPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 300.f;
    static constexpr float POPUP_HEIGHT = 140.f;

    static AdminLoginPopup* create();

private:
    geode::InputNode* passwordInput = nullptr;

    void onRememberPassword(cocos2d::CCObject* sender);

    bool setup() override;
};
