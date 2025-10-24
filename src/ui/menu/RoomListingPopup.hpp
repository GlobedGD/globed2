#pragma once
#include <ui/BasePopup.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class LoadingPopup;

class RoomListingPopup : public BasePopup<RoomListingPopup> {
protected:
    friend class RoomListingCell;
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;
    static const cocos2d::CCSize LIST_SIZE;

    size_t m_roomCount = 0;
    bool m_modActionsOn = false;
    cocos2d::extension::CCScale9Sprite* m_background;
    cue::ListNode* m_list;
    std::optional<MessageListener<msg::RoomListMessage>> m_roomListListener;

    std::optional<MessageListener<msg::RoomStateMessage>> m_successListener;
    std::optional<MessageListener<msg::RoomJoinFailedMessage>> m_failListener;
    LoadingPopup* m_loadingPopup = nullptr;
    uint32_t m_joinedRoomId = 0;

    bool setup() override;
    void updateTitle(size_t roomCount);
    void onReload(CCObject*);
    void toggleModActions(bool enabled);
    void populateList(const std::vector<RoomListingInfo>& rooms);

    void doJoinRoom(uint32_t roomId, bool hasPassword);
    void actuallyJoin(uint32_t roomId, uint64_t passcode);
    void waitForResponse();
    void stopWaiting(std::optional<std::string> failReason);

    void doRemoveCell(RoomListingCell* cell);
};

}
