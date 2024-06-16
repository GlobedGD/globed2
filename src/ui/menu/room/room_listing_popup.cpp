#include "room_listing_popup.hpp"

#include "room_listing_cell.hpp"
#include "room_join_popup.hpp"
#include <data/packets/all.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool RoomListingPopup::setup() {
	this->setTitle("Public Rooms");
    this->setID("room-listing"_spr);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<ScrollLayer>::create(contentSize)
        .anchorPoint(0.f, 0.f)
        .contentSize(contentSize)
        .store(scroll)
        .with([&](auto* scroll) {
            scroll->m_contentLayer->setLayout(
                ColumnLayout::create()
                    ->setGap(5.f)
                    ->setAxisReverse(true)
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setAutoGrowAxis(contentSize.height)
            );

            scroll->setPosition(rlayout.center - scroll->getScaledContentSize() / 2.f);
        })
        .zOrder(2)
        .parent(m_mainLayer);


    nm.addListener<RoomListPacket>(this, [this](std::shared_ptr<RoomListPacket> packet) {
        this->createCells(packet->rooms);
    });

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    Build<CCScale9Sprite>::create("square02_small.png")
        .id("background")
        .contentSize(contentSize)
        .opacity(75)
        .pos(rlayout.center)
        .parent(m_mainLayer)
        .zOrder(1)
        .store(background);

    auto* menu = Build<CCMenu>::create()
        .id("buttons-menu")
        .pos(0.f, 0.f)
        .contentSize(contentSize)
        .parent(m_mainLayer)
        .collect();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->onReload(nullptr);
        })
        .pos(rlayout.bottomRight + CCPoint{-3.f, 3.f})
        .id("reload-btn"_spr)
        .parent(menu);

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            RoomJoinPopup::create()->show();
        })
        .pos(rlayout.bottomLeft + CCPoint{3.f, 3.f})
        .id("add-room-btn"_spr)
        .parent(menu);

    nm.send(RequestRoomListPacket::create());

    return true;
}

void RoomListingPopup::onReload(CCObject* sender) {
    NetworkManager::get().send(RequestRoomListPacket::create());
}

void RoomListingPopup::createCells(std::vector<RoomListingInfo> rlpv) {
    scroll->m_contentLayer->removeAllChildren();
    for (const RoomListingInfo& rlp : rlpv) {
        RoomListingCell* rlc = RoomListingCell::create(rlp, this);
        scroll->m_contentLayer->addChild(rlc);
    }
    scroll->m_contentLayer->updateLayout();
}

void RoomListingPopup::close() {
    this->onClose(nullptr);
}

RoomListingPopup* RoomListingPopup::create() {
	auto ret = new RoomListingPopup();

	if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}

    delete ret;
	return nullptr;
}