#pragma once
#include <defs/geode.hpp>

#include <data/types/room.hpp>

class RoomListingPopup : public geode::Popup<> {
protected:
    friend class RoomListingCell;

    static constexpr float POPUP_WIDTH = 380.f;
    static constexpr float POPUP_HEIGHT = 240.f;
    static constexpr float LIST_WIDTH = POPUP_WIDTH * 0.9f;
    static inline const cocos2d::CCSize contentSize = {LIST_WIDTH, 150.f};

	bool setup() override;

    geode::ScrollLayer* scroll = nullptr;
    cocos2d::extension::CCScale9Sprite* background;

    void onReload(cocos2d::CCObject* sender);
    void createCells(std::vector<RoomListingInfo> rlp);

public:
	static RoomListingPopup* create();
    void close();
};
