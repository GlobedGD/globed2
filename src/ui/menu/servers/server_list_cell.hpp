#pragma once
#include <defs/all.hpp>
#include <managers/game_server.hpp>

class GLOBED_DLL ServerListCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_WIDTH = 358.0f;
    static constexpr float CELL_HEIGHT = 45.0f;
    static constexpr cocos2d::ccColor3B ACTIVE_COLOR = {0, 255, 25};
    static constexpr cocos2d::ccColor3B INACTIVE_COLOR = {255, 255, 255};

    void updateWith(const GameServer& gsview);
    void requestTokenAndConnect();

    static ServerListCell* create(const GameServer& gsview);

    GameServer gsview;

private:
    cocos2d::CCLayer* namePingLayer;
    cocos2d::CCLabelBMFont *labelName, *labelPing, *labelExtra;
    cocos2d::CCMenu* btnMenu;
    CCMenuItemSpriteExtra* btnConnect = nullptr;

    bool init(const GameServer& gsview);
    void onConnect();
    void doConnect();
};