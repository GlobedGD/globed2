#pragma once
#include <defs/all.hpp>
#include <data/types/room.hpp>
#include <data/types/gd.hpp>

class RoomListingCell : public cocos2d::CCLayerColor {
public:
    static constexpr float CELL_HEIGHT = 44.f;

    int accountID;

    void onUser(cocos2d::CCObject* sender);
    bool init(RoomListingInfo);
    static RoomListingCell* create(RoomListingInfo);
};