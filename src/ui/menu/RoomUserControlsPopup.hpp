#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class RoomUserControlsPopup : public BasePopup<RoomUserControlsPopup, int, std::string_view> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    bool m_banned = false;
    cocos2d::CCMenu *m_menu;
    std::string m_username;
    int m_accountId;

    bool setup(int id, std::string_view username);
    void remakeButtons();
};

} // namespace globed