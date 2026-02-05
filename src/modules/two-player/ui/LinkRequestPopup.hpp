#pragma once

#include <ui/BasePopup.hpp>
#include <modules/ui/popups/UserListPopup.hpp>

#include <cue/LoadingCircle.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

class LinkRequestPopup : public BasePopup {
public:
    static LinkRequestPopup* create(int accountId, UserListPopup* popup);

private:
    int m_accountId;
    std::string m_username;
    cue::LoadingCircle* m_loadingCircle;
    cocos2d::CCLabelBMFont* m_loadText;
    cocos2d::CCMenu* m_reqMenu;
    asp::time::Instant m_startedWaiting;
    UserListPopup* m_userListPopup;
    bool m_waiting = false;

    bool init(int accountId, UserListPopup* popup);
    void link(bool p2);
    void update(float dt) override;
    void onClose(CCObject* obj) override;
};

}