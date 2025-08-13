#include "ModUserPopup.hpp"

#include <globed/core/PopupManager.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/UnreadBadge.hpp>
#include <ui/misc/Badges.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace { namespace btnorder {
    static constexpr int Ban = 41;
    static constexpr int Mute = 42;
    static constexpr int RoomBan = 43;
    static constexpr int Whitelist = 45;
    static constexpr int History = 46;
    static constexpr int AdminPassword = 47;
    static constexpr int Kick = 48;
    static constexpr int Notice = 49;
} }

namespace globed {

const CCSize ModUserPopup::POPUP_SIZE { 340.f, 210.f };
constexpr static float btnScale = 0.85f;

bool ModUserPopup::setup(int accountId) {
    m_loadCircle = Build<cue::LoadingCircle>::create()
        .pos(this->center())
        .parent(m_mainLayer);

    auto& nm = NetworkManagerImpl::get();
    m_listener = nm.listen<msg::AdminFetchResponseMessage>([this](const auto& msg) {
        this->onLoaded(msg);
        return ListenerResult::Stop;
    });

    if (accountId != 0) {
        this->startLoadingProfile(fmt::to_string(accountId));
    }

    return true;
}

void ModUserPopup::initUi() {
    // name layout
    m_nameLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(this->fromTop(20.f))
        .parent(m_mainLayer);

    // name label
    Build<CCLabelBMFont>::create(m_score->m_userName.c_str(), "bigFont.fnt")
        .limitLabelWidth(160.f, 0.8f, 0.1f)
        .intoMenuItem([this] {
            globed::openUserProfile(m_score->m_accountID, m_score->m_userID, m_score->m_userName);
        })
        .parent(m_nameLayout)
        .collect();

    this->recreateRoleButton();

    m_nameLayout->updateLayout();

    auto* rootLayout = Build<CCNode>::create()
        .pos(this->fromCenter(0.f, -5.f))
        .anchorPoint(0.5f, 0.5f)
        .contentSize(m_size.width * 0.8f, m_size.height * 0.6f)
        .id("root-layout"_spr)
        .parent(m_mainLayer)
        .collect();

    Build<CCScale9Sprite>::create("square02_001.png", CCRect{0.f, 0.f, 80.f, 80.f})
        .opacity(60)
        .id("bg")
        .parent(rootLayout)
        .with([&](auto spr) {
            auto cs = spr->getParent()->getContentSize();
            cs.height *= 2.f;
            cs.width *= 2.f;
            spr->setContentSize(cs);
            spr->setAnchorPoint({0.f, 0.f});
        })
        .scaleY(0.5f)
        .scaleX(0.5f);

    m_rootMenu = Build<CCMenu>::create()
        .ignoreAnchorPointForPos(false)
        .contentSize(rootLayout->getScaledContentSize() * 0.95f)
        .anchorPoint(0.5f, 0.5f)
        .pos(rootLayout->getScaledContentSize() / 2.f)
        .parent(rootLayout)
        .layout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAutoScale(false)
                    ->setGrowCrossAxis(true));

    this->createMuteAndBanButtons();

    // Whitelist button
    Build<CCSprite>::create(m_data->whitelisted ? "button-admin-unwhitelist.png"_spr : "button-admin-whitelist.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this](CCMenuItemSpriteExtra* btn) {
            geode::createQuickPopup(
                "Confirm",
                m_data->whitelisted ?
                    "Are you sure you want to remove this person from the whitelist?"
                    : "Are you sure you want to whitelist this person?",
                "Cancel",
                "Yes",
                [this, btn](auto, bool confirm) {
                    if (!confirm) return;

                    // TODO: send whitelist message

                    m_data->whitelisted = !m_data->whitelisted;

                    btn->setSprite(
                        Build<CCSprite>::create(
                            m_data->whitelisted ? "button-admin-unwhitelist.png"_spr : "button-admin-whitelist.png"_spr
                            )
                            .scale(btnScale)
                            .collect()
                    );
                }
            );
        })
        .zOrder(btnorder::Whitelist)
        .parent(m_rootMenu);

    // History button
    Build<CCSprite>::create("button-admin-history.png"_spr)
        .scale(btnScale)
        .with([&](auto* btn) {
            // if the user has past punishments, show a little badge with the count
            if (m_data->punishmentCount > 0) {
                Build<UnreadBadge>::create(m_data->punishmentCount)
                    .pos(btn->getScaledContentSize() + CCPoint{1.f, 1.f})
                    .scale(0.7f)
                    .id("count-icon"_spr)
                    .parent(btn);
            }
        })
        .intoMenuItem([this] {
            // TODO: open logs filtering on this person
        })
        .zOrder(btnorder::History)
        .parent(m_rootMenu);

    auto role = NetworkManagerImpl::get().getUserHighestRole();
    // TODO: only show the admin buttons to the admins? but we dont have perms..

    if (true) {
        // Password button
        Build<CCSprite>::create("button-admin-password.png"_spr)
            .scale(btnScale)
            .intoMenuItem([this] {
                // TODO: prompt for password
            })
            .zOrder(btnorder::AdminPassword)
            .parent(m_rootMenu);
    }

    // kick button
    Build<CCSprite>::create("button-admin-kick.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            // TODO: prompt
        })
        .zOrder(btnorder::Kick)
        .parent(m_rootMenu);

    // notice button
    Build<CCSprite>::create("button-admin-notice.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            // TODO: prompt
        })
        .zOrder(btnorder::Notice)
        .parent(m_rootMenu);

    m_rootMenu->updateLayout();
}

void ModUserPopup::recreateRoleButton() {
    if (m_roleModifyButton) {
        m_roleModifyButton->removeFromParent();
        m_roleModifyButton = nullptr;
    }

    CCSprite* badgeIcon = nullptr;

    // assume roles are sorted by priority
    if (!m_data->roles.empty()) {
        badgeIcon = createBadge(m_data->roles[0]);
    } else {
        badgeIcon = CCSprite::create("role-user.png"_spr);
    }

    cue::rescaleToMatch(badgeIcon, BADGE_SIZE * 1.2f);

    m_roleModifyButton = Build(badgeIcon)
        .intoMenuItem([this] {
            // TODO: do nothing if the user has no perms to edit roles

            // TODO: show the role modify popup
        })
        .parent(m_nameLayout);

    m_nameLayout->updateLayout();
}

void ModUserPopup::createMuteAndBanButtons() {
    if (m_banButton) {
        m_banButton->removeFromParent();
    }

    if (m_muteButton) {
        m_muteButton->removeFromParent();
    }

    if (m_roomBanButton) {
        m_roomBanButton->removeFromParent();
    }

    // Ban button
    m_banButton = Build<CCSprite>::create(m_data->activeBan ? "button-admin-unban.png"_spr : "button-admin-ban.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            // TODO: popup
        })
        .zOrder(btnorder::Ban)
        .parent(m_rootMenu);

    auto expSize = m_banButton->getScaledContentSize();

    // Mute button
    m_muteButton = Build<CCSprite>::create(m_data->activeMute ? "button-admin-unmute.png"_spr : "button-admin-mute.png"_spr)
        .with([&](CCSprite* btn) {
            cue::rescaleToMatch(btn, expSize);
        })
        .intoMenuItem([this] {
            // TODO: popup
        })
        .zOrder(btnorder::Mute)
        .parent(m_rootMenu);

    // Room ban button
    m_roomBanButton = Build<CCSprite>::create(m_data->activeRoomBan ? "button-admin-room-unban.png"_spr : "button-admin-room-ban.png"_spr)
        .with([&](CCSprite* btn) {
            cue::rescaleToMatch(btn, expSize);
        })
        .intoMenuItem([this] {
            // TODO: popup
        })
        .zOrder(btnorder::RoomBan)
        .parent(m_rootMenu);

    m_rootMenu->updateLayout();
}

void ModUserPopup::startLoadingProfile(const std::string& query) {
    m_loadCircle->fadeIn();
    m_query = query;

    auto& nm = NetworkManagerImpl::get();
    nm.sendAdminFetchUser(query);
}

void ModUserPopup::onLoaded(const msg::AdminFetchResponseMessage& msg) {
    int queryAccountId = 0;

    if (msg.found) {
        m_data = Data {
            .accountId = msg.accountId,
            .whitelisted = msg.whitelisted,
            .roles = std::move(msg.roles),
            .activeBan = std::move(msg.activeBan),
            .activeRoomBan = std::move(msg.activeRoomBan),
            .activeMute = std::move(msg.activeMute),
        };
        queryAccountId = msg.accountId;
    }

    // query the user in gd
    auto glm = GameLevelManager::get();

    if (queryAccountId != 0) {
        glm->m_userInfoDelegate = this;
        glm->getGJUserInfo(queryAccountId);
    } else {
        glm->m_levelManagerDelegate = this;
        glm->getUsers(GJSearchObject::create(SearchType::Users, m_query));
    }
}

void ModUserPopup::onUserInfoLoaded(geode::Result<GJUserScore*> res) {
    auto glm = GameLevelManager::get();
    glm->m_userInfoDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;

    m_loadCircle->fadeOut();

    if (!res) {
        this->onClose(nullptr);
        globed::alert("Error", res.unwrapErr());
        return;
    }

    m_score = res.unwrap();

    if (!m_data) {
        m_data = Data {
            .accountId = m_score->m_accountID,
            // .. rest are defaults ..
        };
    }

    this->initUi();
}

void ModUserPopup::getUserInfoFinished(GJUserScore* p0) {
    this->onUserInfoLoaded(Ok(p0));
}

void ModUserPopup::getUserInfoFailed(int p0) {
    this->onUserInfoLoaded(Err("failed to fetch user data: {}", p0));
}

void ModUserPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2) {
    if (!levels || levels->count() == 0) {
        this->onUserInfoLoaded(Err("failed to search for user (no results)"));
    } else {
        this->onUserInfoLoaded(Ok(CCArrayExt<GJUserScore>(levels)[0]));
    }
}

void ModUserPopup::loadLevelsFailed(char const* key, int p1) {
    this->onUserInfoLoaded(Err("failed to search for user: error code {}", p1));
}

}