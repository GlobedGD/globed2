#pragma once

#include <modules/ui/popups/UserListPopup.hpp>
#include <ui/BasePopup.hpp>

#include <asp/time/Instant.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class LinkRequestPopup : public BasePopup<LinkRequestPopup, int, UserListPopup *> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    int m_accountId;
    std::string m_username;
    cue::LoadingCircle *m_loadingCircle;
    cocos2d::CCLabelBMFont *m_loadText;
    cocos2d::CCMenu *m_reqMenu;
    asp::time::Instant m_startedWaiting;
    UserListPopup *m_userListPopup;
    bool m_waiting = false;

    bool setup(int accountId, UserListPopup *popup) override;
    void link(bool p2);
    void update(float dt) override;
    void onClose(CCObject *obj) override;
};

} // namespace globed