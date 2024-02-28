#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

using ColorInputCallbackFn = std::function<void(cocos2d::ccColor3B)>;

class GlobedColorInputPopup : public geode::Popup<cocos2d::ccColor3B, ColorInputCallbackFn>, public cocos2d::extension::ColorPickerDelegate {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;

    static GlobedColorInputPopup* create(cocos2d::ccColor3B color, ColorInputCallbackFn fn);

private:
    ColorInputCallbackFn callback;
    cocos2d::extension::CCControlColourPicker* colorPicker;
    geode::TextInput *inputR, *inputG, *inputB, *inputHex;
    cocos2d::ccColor3B storedColor;

    bool setup(cocos2d::ccColor3B color, ColorInputCallbackFn fn) override;

    void colorValueChanged(cocos2d::ccColor3B color) override;
    void updateColors(bool updateWheel = false);
};
