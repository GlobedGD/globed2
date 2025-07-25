#pragma once

#include <ui/BaseLayer.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

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

    bool init();
    void update(float dt);
    void setMenuState(MenuState state);
};

}
