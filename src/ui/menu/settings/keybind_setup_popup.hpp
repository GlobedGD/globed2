#pragma once
#include <defs/geode.hpp>
#include <managers/keybinds.hpp>

class KeybindSetupPopup : public geode::Popup<int, globed::Keybinds> {
public:
    static constexpr float POPUP_WIDTH = 240.f;
    static constexpr float POPUP_HEIGHT = 200.f;

    static KeybindSetupPopup* create(int key, globed::Keybinds keybind);

private:
    bool setup(int key, globed::Keybinds keybind) override;

    void keyDown(cocos2d::enumKeyCodes keyCode) override;

    cocos2d::CCLabelBMFont* m_keybindLabel;
    int key;
};
