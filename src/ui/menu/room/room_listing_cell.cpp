#include "room_listing_cell.hpp"

#include <util/ui.hpp>
#include <net/manager.hpp>
#include "room_password_popup.hpp"
#include "room_listing_popup.hpp"

using namespace geode::prelude;

bool RoomListingCell::init(RoomListingInfo rli) {
    if (!CCLayerColor::init())
        return false;

    accountID = rli.owner.accountId;

    this->setContentSize(ccp(290, CELL_HEIGHT));
    this->setAnchorPoint(ccp(0, 0));

    // background
    Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(this->getContentSize() * 3.f)
        .scale(1.f / 3.f)
        .opacity(67)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    auto* playerBundle = Build<CCMenu>::create()
        .pos(4.f, 21.f)
        .anchorPoint(0.f, 1.f)
        .layout(RowLayout::create()
            ->setGap(4.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisReverse(true)
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
        )
        .parent(this)
        .collect();

    auto* nameLabel = Build<CCLabelBMFont>::create(rli.owner.name.c_str(), "goldFont.fnt")
        .scale(0.5f)
        .id("playername")
        .zOrder(2)
        .collect();

    CCSprite* badgeIcon = util::ui::createBadgeIfSpecial(rli.owner.specialUserData);
    if (badgeIcon) {
        util::ui::rescaleToMatch(badgeIcon, util::ui::BADGE_SIZE);
        // TODO: fix this
        badgeIcon->setPosition(ccp(nameLabel->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 13.5f, nameLabel->getPositionY()));
        badgeIcon->setZOrder(1);
        playerBundle->addChild(badgeIcon);
    }

    auto* usernameButton = Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(RoomListingCell::onUser))
        .pos(3.f, 50.f)
        .zOrder(0)
        .scaleMult(1.1f)
        .parent(playerBundle)
        .collect();

    playerBundle->updateLayout();

    // set the zorder of the button to be the highest, so that when you hold it, the badge and player icon are behind
    // this also *must* be called after updateLayout().
    usernameButton->setZOrder(10);

    CCLabelBMFont* messageTextLabel = Build<CCLabelBMFont>::create(rli.name.c_str(), "bigFont.fnt")
        .pos(4, CELL_HEIGHT - 10.f)
        .limitLabelWidth(200.0f, 0.8f, 0.0f)
        .anchorPoint(ccp(0, 0.5))
        .id("message-text")
        .collect();

    this->addChild(messageTextLabel);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, rli](auto) {
            if (rli.hasPassword) RoomPasswordPopup::create(rli.id)->show();
            else NetworkManager::get().send(JoinRoomPacket::create(rli.id, std::string_view("")));
            if (auto* listing = this->getParent()->getChildByID("room-listing"_spr)) static_cast<RoomListingPopup*>(listing)->close();
        })
        .scaleMult(1.05f)
        .intoNewParent(CCMenu::create())
        .pos(252.f, 22.f)
        .parent(this);

    return true;
}

void RoomListingCell::onUser(CCObject* sender) {
    ProfilePage::create(accountID, GJAccountManager::sharedState()->m_accountID == accountID)->show();
}

RoomListingCell* RoomListingCell::create(RoomListingInfo rli) {
    auto* ret = new RoomListingCell;
    if (ret->init(rli)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
