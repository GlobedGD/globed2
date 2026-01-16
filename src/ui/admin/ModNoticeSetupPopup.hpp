#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModNoticeSetupPopup : public BasePopup<ModNoticeSetupPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    void setupUser(int accountId);

protected:
    geode::TextInput *m_messageInput;
    CCMenuItemToggler *m_userCheckbox;
    CCMenuItemToggler *m_roomCheckbox;
    CCMenuItemToggler *m_levelCheckbox;
    CCMenuItemToggler *m_globalCheckbox;
    geode::TextInput *m_userInput;
    geode::TextInput *m_roomInput;
    geode::TextInput *m_levelInput;
    CCNode *m_inputsContainer;
    CCMenuItemToggler *m_canReplyCheckbox;
    CCMenuItemToggler *m_showSenderCheckbox;
    cocos2d::CCLabelBMFont *m_canReplyLabel;

    bool setup();
    void onSelectMode(int mode, bool on);
    void submit();
};

} // namespace globed