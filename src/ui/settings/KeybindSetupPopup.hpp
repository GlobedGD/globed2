#pragma once

#include <ui/BasePopup.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class KeybindSetupPopup : public BasePopup {
public:
    static KeybindSetupPopup* create(cocos2d::enumKeyCodes key);
    using Callback = geode::Function<void(cocos2d::enumKeyCodes)>;

    void setCallback(Callback&& cb);

protected:
    Callback m_callback;
    cocos2d::CCLabelBMFont* m_keybindLabel;
    cocos2d::enumKeyCodes m_key, m_originalKey;
    bool m_valid = false;

    bool init(cocos2d::enumKeyCodes key);
    void keyDown(cocos2d::enumKeyCodes keyCode, double time) override;
};

}