#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class ModNoticeSetupPopup : public BasePopup {
public:
    static ModNoticeSetupPopup* create();

    void setupUser(int accountId);

protected:
    geode::TextInput* m_messageInput;
    CCMenuItemToggler* m_userCheckbox;
    CCMenuItemToggler* m_roomCheckbox;
    CCMenuItemToggler* m_levelCheckbox;
    CCMenuItemToggler* m_globalCheckbox;
    geode::TextInput* m_userInput;
    geode::TextInput* m_roomInput;
    geode::TextInput* m_levelInput;
    CCNode* m_inputsContainer;
    CCMenuItemToggler* m_canReplyCheckbox;
    CCMenuItemToggler* m_showSenderCheckbox;
    cocos2d::CCLabelBMFont* m_canReplyLabel;

    bool init();
    void onSelectMode(int mode, bool on);
    void submit();
};

}