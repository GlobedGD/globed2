#pragma once
#include <defs/all.hpp>
#include <Geode/ui/TextInput.hpp>
#include <data/packets/all.hpp>
#include "room_listing_cell.hpp"

using namespace geode::prelude;

class RoomListingPopup : public geode::Popup<> {
protected:
    static constexpr float POPUP_WIDTH = 342.f;
    static constexpr float POPUP_HEIGHT = 240.f;
    const cocos2d::CCSize contentSize = {300.f, 150.f};

	bool setup() override;

    geode::ScrollLayer* scroll = nullptr;
    CCScale9Sprite* background;

    void onReload(cocos2d::CCObject* sender);
    void createCells(std::vector<RoomListingInfo> rlp);

public:
	static RoomListingPopup* create();
    void close();
};
