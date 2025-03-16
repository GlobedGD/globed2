#pragma once
#include <defs/geode.hpp>
#include <managers/keybinds.hpp>

class KeybindSetupPopup : public geode::Popup<void*> {
public:
    static constexpr float POPUP_WIDTH = 240.f;
    static constexpr float POPUP_HEIGHT = 200.f;

    static KeybindSetupPopup* create(void* settingStorage);

private:
    bool setup(void* settingStorage) override;

    void keyDown(cocos2d::enumKeyCodes keyCode) override;

    cocos2d::CCLabelBMFont* m_keybindLabel;
    int key;
    bool isValid = false;
    int* settingStorage;
};
