#pragma once

#include <ui/BasePopup.hpp>

namespace globed {

class RoomUserControlsPopup : public BasePopup {
public:
    static RoomUserControlsPopup* create(int accountId, std::string_view username);

private:
    bool m_banned = false;
    cocos2d::CCMenu* m_menu;
    std::string m_username;
    int m_accountId;

    bool init(int id, std::string_view username);
    void remakeButtons();
};

}