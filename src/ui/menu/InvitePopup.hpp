#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class InvitePopup : public BasePopup<InvitePopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    cue::ListNode *m_list;
    std::optional<MessageListener<msg::GlobalPlayersMessage>> m_listener;
    CCMenu *m_rightSideMenu;
    CCMenuItemSpriteExtra *m_searchBtn;
    CCMenuItemSpriteExtra *m_clearSearchBtn;
    std::string m_filter;

    bool setup() override;
    void onLoaded(const std::vector<MinimalRoomPlayer> &players);

    void refresh();
    void promptFilter();
    void setFilter(std::string_view filter);
};

} // namespace globed
