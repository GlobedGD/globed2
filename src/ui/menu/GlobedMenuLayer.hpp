#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BaseLayer.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <asp/time/Instant.hpp>

namespace globed {

enum class MenuState {
    None,
    Disconnected,
    Connecting,
    Connected
};

class GlobedMenuLayer : public BaseLayer {
public:
    static GlobedMenuLayer* create();

    void onServerModified();

private:
    cocos2d::CCMenu* m_connectMenu;
    cocos2d::extension::CCScale9Sprite* m_connectMenuBg;
    CCMenuItemSpriteExtra* m_editServerButton;
    cocos2d::CCLabelBMFont* m_serverNameLabel;
    CCMenuItemSpriteExtra* m_connectButton;
    cocos2d::CCLabelBMFont* m_connStateLabel;
    MenuState m_state = MenuState::None;

    cocos2d::CCNode* m_playerListMenu;
    cue::ListNode* m_playerList;
    cocos2d::CCLabelBMFont* m_roomNameLabel;
    CCMenuItemSpriteExtra* m_roomNameButton;
    cocos2d::CCMenu* m_roomButtonsMenu;
    cocos2d::CCMenu* m_rightSideMenu = nullptr;
    cocos2d::CCMenu* m_leftSideMenu = nullptr;
    cocos2d::CCMenu* m_farLeftMenu = nullptr;
    cocos2d::CCMenu* m_farRightMenu = nullptr;
    std::optional<MessageListener<msg::RoomStateMessage>> m_roomStateListener;
    uint32_t m_roomId = -1;
    std::optional<asp::time::Instant> m_lastRoomUpdate;

    bool init() override;
    void update(float dt) override;
    void setMenuState(MenuState state, bool force = false);

    void keyBackClicked() override;

    void initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, const RoomSettings& settings);
    void updateRoom(const std::string& name, const std::vector<RoomPlayer>& players, const RoomSettings& settings);
    void initRoomButtons();
    void initSideButtons();
    void initFarSideButtons();
    void copyRoomIdToClipboard();

    void onSettings();
};

}
