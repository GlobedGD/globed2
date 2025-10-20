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
    CCMenuItemSpriteExtra* m_searchBtn = nullptr;
    CCMenuItemSpriteExtra* m_clearSearchBtn = nullptr;
    std::optional<MessageListener<msg::RoomStateMessage>> m_roomStateListener;
    std::optional<MessageListener<msg::RoomPlayersMessage>> m_roomPlayersListener;
    uint32_t m_roomId = -1;
    std::optional<asp::time::Instant> m_lastRoomUpdate;
    std::optional<asp::time::Instant> m_lastInteraction;
    std::optional<cue::ScrollPos> m_lastScrollPos;
    std::string m_curFilter;
    bool m_hardRefresh = false;

    bool init() override;
    void update(float dt) override;
    void setMenuState(MenuState state, bool force = false);

    void keyBackClicked() override;
    void onEnter() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

    void initNewRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, size_t playerCount, const RoomSettings& settings);
    void updateRoom(uint32_t id, const std::string& name, const std::vector<RoomPlayer>& players, size_t playerCount, const RoomSettings& settings);
    void updatePlayerList(const std::vector<RoomPlayer>& players);
    bool trySoftRefresh(const std::vector<RoomPlayer>& players);
    void initRoomButtons();
    void initSideButtons();
    void initFarSideButtons();
    void copyRoomIdToClipboard();
    void requestRoomState();
    bool shouldAutoRefresh();
    std::vector<geode::Ref<CCMenuItemSpriteExtra>> createCommonButtons();

    void reloadWithFilter(const std::string& filter);

    void onSettings();
};

}
