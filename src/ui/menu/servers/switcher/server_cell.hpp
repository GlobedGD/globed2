#pragma once
#include <defs/all.hpp>
#include <managers/central_server.hpp>

class ServerSwitcherPopup;

class CentralServerListCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 45.0f;

    static CentralServerListCell* create(const CentralServer& data, int index, ServerSwitcherPopup* parent);

protected:
    bool init(const CentralServer& data, int index, ServerSwitcherPopup* parent);

    cocos2d::CCLabelBMFont* nameLabel;
    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra *btnDelete, *btnModify, *btnSwitch;

    CentralServer data;
    int index;
    ServerSwitcherPopup* parent;
};