#include "room_listing_popup.hpp"

#include "room_listing_cell.hpp"
#include "room_join_popup.hpp"
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <data/packets/all.hpp>

using namespace geode::prelude;

bool RoomListingPopup::setup() {
	this->setTitle("Public Rooms");
    this->setID("room-listing"_spr);

    scroll = ScrollLayer::create(contentSize);
    scroll->setAnchorPoint(ccp(0, 0));
    scroll->ignoreAnchorPointForPosition(false);
    scroll->m_contentLayer->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true)->setAxisAlignment(AxisAlignment::End)->setAutoGrowAxis(contentSize.height));
    scroll->setContentSize(contentSize);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    nm.addListener<RoomListPacket>(this, [this](std::shared_ptr<RoomListPacket> packet) {
        createCells(packet->rooms);
    });

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    background = cocos2d::extension::CCScale9Sprite::create("square02_small.png");
    background->setContentSize(contentSize);
    background->setOpacity(75);
    background->setPosition(ccp(m_title->getPositionX(), 120.f));
    m_mainLayer->addChild(background);
    background->addChild(scroll);

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->onReload(nullptr);
        })
        .pos(340.f, 3.f)
        .id("reload-btn"_spr)
        .intoNewParent(CCMenu::create())
        .contentSize(contentSize)
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            RoomJoinPopup::create()->show();
        })
        .pos(3.f, 3.f)
        .id("add-room-btn"_spr)
        .intoNewParent(CCMenu::create())
        .contentSize(contentSize)
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    this->setTouchEnabled(true);
    CCTouchDispatcher::get()->addTargetedDelegate(this, -129, true);
    CCTouchDispatcher::get()->addTargetedDelegate(scroll, -130, true);

    nm.send(RequestRoomListPacket::create());

    return true;
}

void RoomListingPopup::onReload(CCObject* sender) {
    NetworkManager::get().send(RequestRoomListPacket::create());
}

void RoomListingPopup::createCells(std::vector<RoomListingInfo> rlpv) {
    scroll->m_contentLayer->removeAllChildren();
    for (RoomListingInfo rlp : rlpv) {
        RoomListingCell* rlc = RoomListingCell::create(rlp);
        rlc->setContentSize({contentSize.width, 50.f});
        scroll->m_contentLayer->addChild(rlc);
    }
    scroll->m_contentLayer->updateLayout();
}

void RoomListingPopup::close() {
    this->onClose(nullptr);
}

RoomListingPopup* RoomListingPopup::create() {
	auto ret = new RoomListingPopup;

	if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, "GJ_square01.png")) {
		ret->autorelease();
		return ret;
	}

    delete ret;
	return nullptr;
}