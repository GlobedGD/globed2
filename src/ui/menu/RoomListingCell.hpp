#pragma once

#include <globed/core/data/RoomListingInfo.hpp>
#include "RoomListingPopup.hpp"

#include <Geode/Geode.hpp>

namespace globed {

class RoomListingCell : public cocos2d::CCLayerColor {
public:
    static constexpr float CELL_HEIGHT = 37.f;

    static RoomListingCell* create(const RoomListingInfo& info, RoomListingPopup* popup);
    void toggleModActions(bool enabled);
    size_t getPlayerCount();

private:
    RoomListingPopup* m_popup;
    RoomListingInfo m_info;
    cocos2d::CCMenu* m_rightMenu;
    CCMenuItemSpriteExtra* m_rightButton = nullptr;
    bool m_modActions = false;

    bool init(const RoomListingInfo& info, RoomListingPopup* popup);
    void recreateButton();
    void removeMeFromList();
};

}
