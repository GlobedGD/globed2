#include "player_list_cell.hpp"

#include "room_popup.hpp"
#include "download_level_popup.hpp"
#include <data/packets/client/room.hpp>
#include <hooks/level_select_layer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <managers/admin.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool PlayerListCell::init(const PlayerRoomPreviewAccountData& data, bool forInviting) {
    if (!CCLayer::init()) return false;
    this->data = data;

    auto* gm = GameManager::get();

    Build<GlobedSimplePlayer>::create(GlobedSimplePlayer::Icons {
            .type = IconType::Cube,
            .id = data.cube,
            .color1 = data.color1,
            .color2 = data.color2,
            .color3 = data.glowColor,
        })
        .scale(0.65f)
        .parent(this)
        .anchorPoint(0.5f, 0.5f)
        .pos(25.f, CELL_HEIGHT / 2.f)
        .id("player-icon"_spr)
        .store(simplePlayer);

    // name label

    RichColor nameColor = util::ui::getNameRichColor(data.specialUserData);

    CCMenu* badgeWrapper = Build<CCMenu>::create()
        .pos(simplePlayer->getPositionX() + simplePlayer->getScaledContentSize().width / 2 + 10.f, CELL_HEIGHT / 2)
        .layout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start)->setAutoScale(false))
        .anchorPoint(0.f, 0.5f)
        .contentSize(RoomPopup::LIST_WIDTH, CELL_HEIGHT)
        .scale(1.f)
        .parent(this)
        .id("badge-wrapper"_spr)
        .collect();

    float labelWidth;
    auto* label = Build<CCLabelBMFont>::create(data.name.c_str(), "bigFont.fnt")
        .limitLabelWidth(170.f, 0.6f, 0.1f)
        .with([&labelWidth, &nameColor](CCLabelBMFont* label) {
            label->setScale(label->getScale() * 0.9f);
            labelWidth = label->getScaledContentSize().width;
            util::ui::animateLabelColorTint(label, nameColor);
        })
        .intoMenuItem([this] {
            this->onOpenProfile(nullptr);
        })
        .pos(simplePlayer->getPositionX() + labelWidth / 2.f + 25.f, CELL_HEIGHT / 2.f)
        .scaleMult(1.1f)
        .parent(badgeWrapper)
        .collect();

    auto badge = util::ui::createBadgeIfSpecial(data.specialUserData);
    if (badge) {
        util::ui::rescaleToMatch(badge, util::ui::BADGE_SIZE);
        badgeWrapper->addChild(badge);
    }

    badgeWrapper->updateLayout();

    const float pad = 5.f;
    Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::End))
        .anchorPoint(0.f, 0.5f)
        .pos(pad, CELL_HEIGHT / 2.f)
        .contentSize(RoomPopup::LIST_WIDTH - pad * 2, CELL_HEIGHT)
        .parent(this)
        .store(menu);

    if (forInviting) {
        this->createInviteButton();
    } else {
        this->createJoinButton();
    }

    if (AdminManager::get().authorized()) {
        this->createAdminButton();
    }

    return true;
}

void PlayerListCell::createInviteButton() {
    Build<CCSprite>::createSpriteName("icon-invite.png"_spr)
        .scale(0.85f)
        .intoMenuItem([accountId = this->data.accountId](auto) {
            NetworkManager::get().send(RoomSendInvitePacket::create(accountId));
        })
        .scaleMult(1.25f)
        .store(inviteButton)
        .parent(menu);

    menu->updateLayout();
}

void PlayerListCell::createJoinButton() {
    if (this->data.levelId == 0) {
        return;
    }

    Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
        .scale(0.30f)
        .intoMenuItem([levelId = this->data.levelId](auto) {
            auto* glm = GameLevelManager::sharedState();
            auto mlevel = glm->m_mainLevels->objectForKey(std::to_string(levelId));
            bool isMainLevel = std::find(HookedLevelSelectLayer::MAIN_LEVELS.begin(), HookedLevelSelectLayer::MAIN_LEVELS.end(), levelId) != HookedLevelSelectLayer::MAIN_LEVELS.end();

            if (mlevel != nullptr) {
                // if its a classic main level go to that page in LevelSelectLayer
                if (isMainLevel) {
                    util::ui::switchToScene(LevelSelectLayer::scene(levelId - 1));
                    return;
                }

                // otherwise we just go right to playlayer
                auto level = static_cast<HookedGJGameLevel*>(glm->getMainLevel(levelId, false));
                level->m_fields->shouldTransitionWithPopScene = true;
                util::ui::switchToScene(PlayLayer::scene(level, false, false));
                return;
            }

            if (auto popup = DownloadLevelPopup::create(levelId)) {
                popup->show();
            }
        })
        .pos(RoomPopup::LIST_WIDTH - 30.f, CELL_HEIGHT / 2.f)
        .scaleMult(1.1f)
        .store(playButton)
        .parent(menu);

    menu->updateLayout();
}

void PlayerListCell::createAdminButton() {
    // admin menu button
    Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
        .scale(0.525f)
        .intoMenuItem([this](auto) {
            AdminManager::get().openUserPopup(data);
        })
        .parent(menu)
        .zOrder(-3)
        .id("admin-button"_spr);

    menu->updateLayout();
}

void PlayerListCell::onOpenProfile(cocos2d::CCObject*) {
    GameLevelManager::sharedState()->storeUserName(data.userId, data.accountId, data.name);
    ProfilePage::create(data.accountId, false)->show();
}

PlayerListCell* PlayerListCell::create(const PlayerRoomPreviewAccountData& data, bool forInviting) {
    auto ret = new PlayerListCell;
    if (ret->init(data, forInviting)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}