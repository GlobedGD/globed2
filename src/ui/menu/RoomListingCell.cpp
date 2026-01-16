#include "RoomListingCell.hpp"
#include <globed/core/actions.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/NameLabel.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;

namespace globed {

namespace {
    namespace btnorder {
        constexpr int Join = 43;
        constexpr int Settings = 44;
        constexpr int PlayerCount = 45;
    }
}

static bool hasNameColor(const SpecialUserData& sud) {
    if (sud.nameColor) return true;

    for (uint8_t id : sud.roleIds) {
        auto role = NetworkManagerImpl::get().findRole(id);
        if (role && !role->hide) {
            return true;
        }
    }

    return false;
}

bool RoomListingCell::init(const RoomListingInfo& info, RoomListingPopup* popup) {
    if (!CCLayerColor::init()) {
        return false;
    }

    m_popup = popup;
    m_info = info;

    this->setContentSize({RoomListingPopup::LIST_SIZE.width, CELL_HEIGHT});
    // this->setAnchorPoint({0.f, 0.f});

    const float ScaleMult = 3.f;
    Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(this->getContentSize() * ScaleMult)
        .scale(1.f / ScaleMult)
        .opacity(67)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    auto playerMenu = Build<CCMenu>::create()
        .pos(4.f, 15.5f)
        .anchorPoint(0.f, 1.f)
        .layout(SimpleRowLayout::create()
            ->setGap(3.f)
            ->setMainAxisScaling(AxisScaling::Fit)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
        )
        .parent(this)
        .collect();

    auto playerIcon = Build(info.roomOwner.createIcon())
        .scale(0.43f)
        .parent(playerMenu)
        .zOrder(-1)
        .collect();

    bool customNameColor = info.roomOwner.specialUserData && hasNameColor(*info.roomOwner.specialUserData);

    auto nameButton = Build(NameLabel::create(info.roomOwner.accountData.username, customNameColor ? "bigFont.fnt" : "goldFont.fnt"))
        .scale(customNameColor ? 0.68f : 0.7f)
        .id("player-name")
        .pos(3.f, 50.f)
        .zOrder(0)
        .parent(playerMenu)
        .collect();

    nameButton->makeClickable([this](auto) {
        globed::openUserProfile(m_info.roomOwner);
    });

    if (info.roomOwner.specialUserData) {
        nameButton->updateWithRoles(*info.roomOwner.specialUserData);
    }

    playerMenu->updateLayout();
    nameButton->setPositionY(nameButton->getPositionY() + 1.f); // move text slightly up

    // set the zorder of the button to be the highest, so that when you hold it, the badge and player icon are behind
    // this also *must* be called after updateLayout().
    nameButton->setZOrder(10);

    /// Room name

    auto roomNameLayout = Build<CCMenu>::create()
        .pos(4.f, CELL_HEIGHT - 10.f)
        .anchorPoint(0.f, 0.5f)
        .layout(SimpleRowLayout::create()
            ->setGap(3.f)
            ->setMainAxisScaling(AxisScaling::ScaleDownGaps)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
        )
        .contentSize(RoomListingPopup::LIST_SIZE.width / 2.f, 0.f)
        .parent(this)
        .collect();

    auto roomNameLabel = Build<Label>::create(info.roomName.c_str(), "bigFont.fnt")
        .id("room-name")
        .parent(roomNameLayout)
        .collect();

    // button for extended data
    m_extInfoButton = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .scale(0.6f)
        .intoMenuItem([this] {
            this->showExtendedData();
        })
        .id("ext-data-btn")
        .visible(false)
        .parent(roomNameLayout);

    m_rightMenu = Build<CCMenu>::create()
        .layout(SimpleRowLayout::create()
            ->setGap(4.f)
            ->setMainAxisDirection(AxisDirection::RightToLeft)
            ->setCrossAxisScaling(AxisScaling::Fit)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
        )
        .anchorPoint(1.f, 0.5f)
        .pos(RoomListingPopup::LIST_SIZE.width - 3.f, CELL_HEIGHT / 2.f)
        .contentSize(this->getContentWidth() / 2.f, this->getContentHeight())
        .parent(this);

    auto roomSettingsMenu = Build<CCMenu>::create()
        .layout(SimpleRowLayout::create()
            ->setGap(1.f)
            ->setMainAxisDirection(AxisDirection::RightToLeft)
            ->setCrossAxisScaling(AxisScaling::Fit)
        )
        .contentSize(77.f, this->getContentHeight())
        .parent(m_rightMenu)
        .zOrder(btnorder::Settings)
        .collect();

    // add join button
    this->recreateButton();

    // lock icon, represents locked rooms
    auto lockSpr = Build<CCSprite>::create("room-icon-lock.png"_spr)
        .scale(0.38f)
        .opacity(info.hasPassword ? 255 : 80)
        .intoMenuItem(+[] {
            globed::alert("Locked room", "This room requires a password to join.");
        })
        .parent(roomSettingsMenu)
        .collect();

    lockSpr->setEnabled(info.hasPassword);

    // collision icon
    auto* collisionBtn = Build<CCSprite>::create("room-icon-collision.png"_spr)
        .with([&](auto* s) {
            cue::rescaleToMatch(s, lockSpr->getScaledContentSize());
        })
        .opacity(info.settings.collision ? 255 : 80)
        .intoMenuItem(+[] {
            globed::alert("Collision",
                "This room has collision enabled, meaning you can collide with other players.\n\n"
                "<cy>Note: this means the room has safe mode, making it impossible to make progress on levels.</c>"
            );
        })
        .parent(roomSettingsMenu)
        .collect();

    collisionBtn->setEnabled(info.settings.collision);

    // deathlink icon
    auto* deathlinkBtn = Build<CCSprite>::create("room-icon-deathlink.png"_spr)
        .with([&](auto* s) {
            cue::rescaleToMatch(s, lockSpr->getScaledContentSize());
        })
        .opacity(info.settings.deathlink ? 255 : 80)
        .intoMenuItem(+[] {
            globed::alert("Death Link",
                "This room has Death Link enabled, which means that if a player dies, everyone in the level dies as well. "
                "<cy>Originally invented by </c> <cg>Alphalaneous</c>."
            );
        })
        .parent(roomSettingsMenu)
        .collect();

    deathlinkBtn->setEnabled(info.settings.deathlink);

    // 2 player icon
    auto* twoPlayerBtn = Build<CCSprite>::create("room-icon-2p.png"_spr)
        .with([&](auto* s) {
            cue::rescaleToMatch(s, lockSpr->getScaledContentSize());
        })
        .opacity(info.settings.twoPlayerMode ? 255 : 80)
        .intoMenuItem(+[] {
            globed::alert("2 Player",
                "This room has 2 Player enabled, which means you can play 2-player levels with a remote friend."
            );
        })
        .parent(roomSettingsMenu)
        .collect();

    twoPlayerBtn->setEnabled(info.settings.twoPlayerMode);

    auto playerCountWrapper = Build<CCNode>::create()
        .layout(SimpleRowLayout::create()->setGap(1.f)->setCrossAxisScaling(AxisScaling::Fit))
        .parent(m_rightMenu)
        .zOrder(btnorder::PlayerCount)
        .collect();

    std::string playerCountText = info.settings.playerLimit == 0
        ? fmt::to_string(info.playerCount)
        : fmt::format("{}/{}", info.playerCount, info.settings.playerLimit);

    auto playerCountLabel = Build<Label>::create(playerCountText, "bigFont.fnt")
        .with([&](auto lbl) { lbl->limitLabelWidth(35.f, 0.35f, 0.1f); })
        .parent(playerCountWrapper)
        .collect();

    auto playerCountIcon = Build<CCSprite>::create("icon-person.png"_spr)
        .with([&](auto* s) {
            cue::rescaleToMatch(s, CCSize {11.52f, 11.52f});
        })
        .parent(playerCountWrapper)
        .collect();

    playerCountWrapper->setContentWidth(1.f + playerCountIcon->getScaledContentWidth() + playerCountLabel->getScaledContentWidth());
    playerCountWrapper->updateLayout();
    roomSettingsMenu->updateLayout();
    m_rightMenu->updateLayout();

    // update width of the room name accordingly, so it fits
    roomNameLabel->limitLabelWidth(200.f - playerCountWrapper->getContentWidth(), 0.48f, 0.1f);
    roomNameLayout->updateLayout();

    // add a background
    float sizeScale = 3.5f;
    auto* settingsBg = Build<CCScale9Sprite>::create("square02_001.png")
        .opacity(67)
        .zOrder(-1)
        .contentSize(roomSettingsMenu->getScaledContentSize() * sizeScale + CCPoint{6.f, 8.f})
        .scaleX(1.f / sizeScale)
        .scaleY(1.f / sizeScale)
        .parent(roomSettingsMenu)
        .anchorPoint(0.5f, 0.5f)
        .pos(roomSettingsMenu->getScaledContentSize() / 2.f)
        .collect();

    Build<CCScale9Sprite>::create("square02_001.png")
        .opacity(67)
        .zOrder(-1)
        .contentSize(playerCountWrapper->getScaledContentSize().width * sizeScale + 8.f, settingsBg->getContentHeight())
        .scaleX(1.f / sizeScale)
        .scaleY(1.f / sizeScale)
        .parent(playerCountWrapper)
        .anchorPoint(0.5f, 0.5f)
        .pos(playerCountWrapper->getScaledContentSize() / 2.f);

    return true;
}

void RoomListingCell::recreateButton() {
    cue::resetNode(m_rightButton);

    if (m_modActions) {
        m_rightButton = Build<ButtonSprite>::create("Close", "bigFont.fnt", "GJ_button_06.png", 0.8f)
            .scale(0.7f)
            .intoMenuItem([this] {
                NetworkManagerImpl::get().sendAdminCloseRoom(m_info.roomId);
                this->removeMeFromList();
            })
            .scaleMult(1.15f)
            .zOrder(btnorder::Join)
            .parent(m_rightMenu);
    } else {
        m_rightButton = Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .scale(0.7f)
            .intoMenuItem([this] {
                m_popup->doJoinRoom(m_info.roomId, m_info.hasPassword);
            })
            .scaleMult(1.15f)
            .zOrder(btnorder::Join)
            .parent(m_rightMenu);
    }

    m_rightMenu->updateLayout();
}

size_t RoomListingCell::getPlayerCount() {
    return m_info.playerCount;
}

void RoomListingCell::toggleModActions(bool enabled) {
    if (m_modActions == enabled) {
        return; // no change
    }

    m_modActions = enabled;
    m_extInfoButton->setVisible(enabled);

    this->recreateButton();
}

void RoomListingCell::removeMeFromList() {
    m_popup->doRemoveCell(this);
}

void RoomListingCell::showExtendedData() {
    globed::alertFormat(
        "Room Info",
        "Room ID: {}\nOwner ID: {}\nOriginal owner ID: {}",
        m_info.roomId,
        m_info.roomOwner.accountData.accountId,
        m_info.originalOwnerId
    );
}

RoomListingCell* RoomListingCell::create(const RoomListingInfo& info, RoomListingPopup* popup) {
    auto ret = new RoomListingCell;
    if (ret->init(info, popup)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
