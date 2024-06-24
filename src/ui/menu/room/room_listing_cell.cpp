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

    // log::debug("rli: {}", rli.id);

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
            ->setGap(3.f)
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

    auto* roomNameLayout = Build<CCNode>::create()
        .pos(4, CELL_HEIGHT - 10.f)
        .anchorPoint(0.f, 0.5f)
        .layout(
            RowLayout::create()
                ->setGap(3.f)
                ->setAutoScale(false)
                ->setAxisAlignment(AxisAlignment::Start)
        )
        .contentSize(RoomListingPopup::LIST_WIDTH, 0.f)
        .parent(this)
        .collect();

    CCLabelBMFont* roomNameLabel = Build<CCLabelBMFont>::create(rli.name.c_str(), "bigFont.fnt")
        .limitLabelWidth(185.0f, 0.48f, 0.1f)
        .id("message-text")
        .parent(roomNameLayout)
        .collect();

    roomNameLayout->updateLayout();

    auto* rightMenu = Build<CCMenu>::create()
        .layout(
            RowLayout::create()
                    ->setAxisReverse(true)
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setAutoScale(false)
                    ->setGap(2.f)
        )
        .anchorPoint(1.f, 0.5f)
        .pos(RoomListingPopup::LIST_WIDTH - 3.f, CELL_HEIGHT / 2.f)
        .contentSize(this->getContentSize())
        .parent(this)
        .collect();

    // join button
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
        .scaleMult(1.15f)
        .parent(rightMenu);

    // lock button
    auto* lockSpr = Build<CCSprite>::createSpriteName("GJLargeLock_001.png")
        .scale(0.24f)
        .opacity(rli.hasPassword ? 255 : 80)
        .parent(rightMenu)
        .collect();

    // collision icon
    Build<CCSprite>::createSpriteName("room-icon-collision.png"_spr)
        .with([&](auto* s) {
            util::ui::rescaleToMatch(s, lockSpr->getScaledContentSize() * 1.2f);
        })
        .opacity(rli.settings.flags.collision ? 255 : 80)
        .parent(rightMenu);

    auto* playerCountWrapper = Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(1.f)->setAutoScale(false))
        .parent(rightMenu)
        .collect();

    // player count number
    int playerCount = rli.playerCount;
    std::string playerCountText = rli.settings.playerLimit == 0 ? std::to_string(playerCount) : fmt::format("{}/{}", playerCount, rli.settings.playerLimit);

    auto* playerCountLabel = Build<CCLabelBMFont>::create(playerCountText.c_str(), "bigFont.fnt")
        .scale(0.35f)
        .parent(playerCountWrapper)
        .collect();

    // player count icon
    auto* playerCountIcon = Build<CCSprite>::createSpriteName("icon-person.png"_spr)
        .with([&](auto* p) {
            util::ui::rescaleToMatch(p, util::ui::BADGE_SIZE_SMALL * 0.9f);
        })
        .parent(playerCountWrapper)
        .collect();

    playerCountWrapper->setContentWidth(1.f + playerCountIcon->getScaledContentSize().width + playerCountLabel->getScaledContentSize().width);

    playerCountWrapper->updateLayout();

    rightMenu->updateLayout();

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
