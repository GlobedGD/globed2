#pragma once
#include <ui/BasePopup.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class RoomListingPopup : public BasePopup<RoomListingPopup> {
protected:
    friend class RoomListingCell;
    friend class BasePopup;
    static cocos2d::CCSize POPUP_SIZE;
    static cocos2d::CCSize LIST_SIZE;

    size_t m_roomCount = 0;
    bool m_modActionsOn = false;
    cocos2d::extension::CCScale9Sprite* m_background;
    cue::ListNode* m_list;
    std::optional<MessageListener<msg::RoomListMessage>> m_roomListListener;

    bool setup() override;
    void updateTitle(size_t roomCount);
    void onReload(CCObject*);
    void toggleModActions(bool enabled);
    void populateList(const std::vector<RoomListingInfo>& rooms);
};

}
