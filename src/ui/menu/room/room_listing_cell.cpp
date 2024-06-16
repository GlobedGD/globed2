#include "room_listing_cell.hpp"

#include "room_password_popup.hpp"
#include "room_listing_popup.hpp"
#include <ui/general/simple_player.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool RoomListingCell::init(const RoomListingInfo& rli, RoomListingPopup* parent) {
    if (!CCLayerColor::init())
        return false;

    this->parent = parent;

    accountID = rli.owner.accountId;

    this->setContentSize({RoomListingPopup::LIST_WIDTH, CELL_HEIGHT});
    this->setAnchorPoint({0.f, 0.f});

    // background
    const float scaleMult = 3.f;
    Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(this->getContentSize() * scaleMult)
        .scale(1.f / scaleMult)
        .opacity(67)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    auto* playerBundle = Build<CCMenu>::create()
        .pos(4.f, 17.f)
        .anchorPoint(0.f, 1.f)
        .layout(RowLayout::create()
            ->setGap(2.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisReverse(true)
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
        )
        .parent(this)
        .collect();

    auto* playerIcon = Build<GlobedSimplePlayer>::create(rli.owner)
        .scale(0.35f)
        .parent(playerBundle)
        .zOrder(-1)
        .collect();

    auto* nameLabel = Build<CCLabelBMFont>::create(rli.owner.name.c_str(), "goldFont.fnt")
        .scale(0.5f)
        .id("playername")
        .zOrder(2)
        .collect();

    CCSprite* badgeIcon = util::ui::createBadgeIfSpecial(rli.owner.specialUserData);
    if (badgeIcon) {
        util::ui::rescaleToMatch(badgeIcon, util::ui::BADGE_SIZE_SMALL);
        // TODO: fix this
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

    CCLabelBMFont* roomNameLabel = Build<CCLabelBMFont>::create(rli.name.c_str(), "bigFont.fnt")
        .pos(4.f, CELL_HEIGHT - 10.f)
        .limitLabelWidth(275.0f, 0.5f, 0.1f)
        .anchorPoint(0, 0.5)
        .id("message-text")
        .collect();

    this->addChild(roomNameLabel);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.7f)
        .intoMenuItem([this, rli](auto) {
            if (rli.hasPassword) {
                RoomPasswordPopup::create(rli.id)->show();
                return;
            }

            NetworkManager::get().send(JoinRoomPacket::create(rli.id, std::string_view("")));
            this->parent->close();
        })
        .with([&](auto* btn) {
            btn->setPosition(RoomListingPopup::LIST_WIDTH - btn->getScaledContentSize().width / 2.f - 3.f, CELL_HEIGHT / 2.f);
        })
        .scaleMult(1.15f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    return true;
}

void RoomListingCell::onUser(CCObject* sender) {
    ProfilePage::create(accountID, GJAccountManager::sharedState()->m_accountID == accountID)->show();
}

RoomListingCell* RoomListingCell::create(const RoomListingInfo& rli, RoomListingPopup* parent) {
    auto* ret = new RoomListingCell();
    if (ret->init(rli, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
