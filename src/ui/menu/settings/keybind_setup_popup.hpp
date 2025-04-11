#pragma once
#include <defs/geode.hpp>
#include <managers/keybinds.hpp>

class KeybindSetupPopup : public geode::Popup<cocos2d::enumKeyCodes, std::function<void(cocos2d::enumKeyCodes)>> {
public:
    static constexpr float POPUP_WIDTH = 240.f;
    static constexpr float POPUP_HEIGHT = 200.f;
    using Callback = std::function<void(cocos2d::enumKeyCodes)>;

    static KeybindSetupPopup* create(cocos2d::enumKeyCodes initialKey, Callback callback);

private:
    bool setup(cocos2d::enumKeyCodes initialKey, Callback callback) override;

    void keyDown(cocos2d::enumKeyCodes keyCode) override;

    Callback callback;
    cocos2d::CCLabelBMFont* m_keybindLabel;
    cocos2d::enumKeyCodes key, originalKey;
    bool isValid = false;
};
