#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BaseLayer.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

enum class MenuState {
    Disconnected,
    Connecting,
    Connected
};

class GlobedMenuLayer : public BaseLayer {
public:
    static GlobedMenuLayer* create();

private:
    cocos2d::CCMenu* m_connectMenu;
    CCMenuItemSpriteExtra* m_editServerButton;
    CCMenuItemSpriteExtra* m_connectButton;
    cocos2d::CCLabelBMFont* m_connStateLabel;
    MenuState m_state;

    cocos2d::CCNode* m_playerListMenu;
    cue::ListNode* m_playerList;
    cocos2d::CCLabelBMFont* m_roomNameLabel;
    CCMenuItemSpriteExtra* m_roomNameButton;
    cocos2d::CCMenu* m_roomButtonsMenu;
    std::optional<MessageListener<msg::RoomStateMessage>> m_roomStateListener;
    std::optional<MessageListener<msg::RoomJoinFailedMessage>> m_roomJoinFailedListener;
    std::optional<MessageListener<msg::RoomCreateFailedMessage>> m_roomCreateFailedListener;
    uint32_t m_roomId = -1;
    std::optional<asp::time::Instant> m_lastRoomUpdate;

    bool init();
    void update(float dt);
    void setMenuState(MenuState state);

    void initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players);
    void initRoomButtons();
    void copyRoomIdToClipboard();
};

}
