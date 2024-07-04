#pragma once
#include <defs/all.hpp>

#include <data/types/room.hpp>

class RoomListingPopup;

class RoomListingCell : public cocos2d::CCLayerColor {
public:
    static constexpr float CELL_HEIGHT = 35.f;

    void onUser(cocos2d::CCObject* sender);
    bool init(const RoomListingInfo& info, RoomListingPopup* parent);
    static RoomListingCell* create(const RoomListingInfo& info, RoomListingPopup* parent);

private:
    friend class RoomListingPopup;

    int playerCount;
    PlayerPreviewAccountData ownerData;
    RoomListingPopup* parent;
};