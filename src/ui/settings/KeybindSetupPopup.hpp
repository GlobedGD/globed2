#pragma once

#include <ui/BasePopup.hpp>

#include <std23/move_only_function.h>

namespace globed {

class KeybindSetupPopup : public BasePopup<KeybindSetupPopup, cocos2d::enumKeyCodes> {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    using Callback = std23::move_only_function<void(cocos2d::enumKeyCodes)>;

    void setCallback(Callback &&cb);

protected:
    Callback m_callback;
    cocos2d::CCLabelBMFont *m_keybindLabel;
    cocos2d::enumKeyCodes m_key, m_originalKey;
    bool m_valid = false;

    bool setup(cocos2d::enumKeyCodes key) override;
    void keyDown(cocos2d::enumKeyCodes keyCode) override;
};

} // namespace globed