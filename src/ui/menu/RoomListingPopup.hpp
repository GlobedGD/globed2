#pragma once
#include <ui/BasePopup.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>

#include <cue/ListNode.hpp>

namespace globed {

class LoadingPopup;
class RoomListingCell;

class RoomListingPopup : public BasePopup<RoomListingPopup> {
protected:
    friend class RoomListingCell;
    friend class BasePopup;
    static const cocos2d::CCSize POPUP_SIZE;
    static const cocos2d::CCSize LIST_SIZE;

    std::string m_filter;
    size_t m_roomCount = 0;
    uint32_t m_loadedPages = 0;
    uint32_t m_page = 0;
    uint32_t m_maxPages = 0;
    uint32_t m_totalRooms = 0;
    bool m_modActionsOn = false;
    bool m_loading = false;
    cocos2d::extension::CCScale9Sprite* m_background;
    cue::ListNode* m_list;
    CCMenuItemSpriteExtra* m_searchBtn = nullptr;
    CCMenuItemSpriteExtra* m_clearSearchBtn = nullptr;
    std::vector<std::vector<RoomListingInfo>> m_allRooms;
    std::optional<MessageListener<msg::RoomListMessage>> m_roomListListener;

    std::optional<MessageListener<msg::RoomStateMessage>> m_successListener;
    std::optional<MessageListener<msg::RoomJoinFailedMessage>> m_failListener;
    LoadingPopup* m_loadingPopup = nullptr;
    uint32_t m_joinedRoomId = 0;
    uint64_t m_joinedRoomPasscode = 0;

    bool setup() override;
    void updateTitle(size_t roomCount);
    void onReload();
    void requestRooms();
    void toggleModActions(bool enabled);
    void populateList();
    void onPageLoaded(const std::vector<RoomListingInfo>& rooms, uint32_t page);
    void switchPage(int delta);

    void setFilter(std::string_view filter);
    void promptFilter();

    void doJoinRoom(uint32_t roomId, bool hasPassword);
    void actuallyJoin(uint32_t roomId, uint64_t passcode);
    void waitForResponse();
    void stopWaiting(std::optional<std::string> failReason, bool dontClose = false);

    void doRemoveCell(RoomListingCell* cell);
};

}
