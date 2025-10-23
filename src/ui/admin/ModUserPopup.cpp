#include "ModUserPopup.hpp"
#include "ModPunishPopup.hpp"
#include "ModRoleModifyPopup.hpp"
#include "ModNoticeSetupPopup.hpp"
#include "ModAuditLogPopup.hpp"

#include <globed/core/PopupManager.hpp>
#include <globed/core/actions.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/misc/UnreadBadge.hpp>
#include <ui/misc/Badges.hpp>
#include <ui/misc/InputPopup.hpp>

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
    auto& nm = NetworkManagerImpl::get();
    m_listener = nm.listen<msg::AdminFetchResponseMessage>([this](const auto& msg) {
        this->onLoaded(msg);
        return ListenerResult::Stop;
    });

    m_resultListener = nm.listen<msg::AdminResultMessage>([this](const auto& msg) {
        if (!msg.success) {
            globed::alert("Error", fmt::format("Received error from the server: <cy>{}</c>", msg.error));
        }

        return ListenerResult::Stop;
    });
    m_resultListener->setPriority(10000);

    // Loading has multiple steps:
    // 1. Fetch various data from the globed server (punishments, roles, etc.)
    // 2. Search for the user on the GD server to get their up-to-date account data

    if (accountId != 0) {
        this->startLoadingProfile(accountId);
    }

    return true;
}

void ModUserPopup::initUi() {
    cue::resetNode(m_loadCircle);
    cue::resetNode(m_nameLayout);
    cue::resetNode(m_rootLayout);

    // name layout
    m_nameLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(this->fromTop(20.f))
        .id("name-layout"_spr)
        .parent(m_mainLayer);

    // name label
    Build<CCLabelBMFont>::create(m_score->m_userName.c_str(), "bigFont.fnt")
        .limitLabelWidth(160.f, 0.8f, 0.1f)
        .intoMenuItem([this] {
            globed::openUserProfile(m_score->m_accountID, m_score->m_userID, m_score->m_userName);
        })
        .scaleMult(1.15f)
        .parent(m_nameLayout)
        .collect();

    this->recreateRoleButton();

    m_nameLayout->updateLayout();

    m_rootLayout = Build<CCNode>::create()
        .pos(this->fromCenter(0.f, -5.f))
        .anchorPoint(0.5f, 0.5f)
        .contentSize(m_size.width * 0.8f, m_size.height * 0.6f)
        .id("root-layout"_spr)
        .parent(m_mainLayer)
        .collect();

    Build<CCScale9Sprite>::create("square02_001.png", CCRect{0.f, 0.f, 80.f, 80.f})
        .opacity(60)
        .id("bg")
        .parent(m_rootLayout)
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
        .contentSize(m_rootLayout->getScaledContentSize() * 0.95f)
        .anchorPoint(0.5f, 0.5f)
        .pos(m_rootLayout->getScaledContentSize() / 2.f)
        .parent(m_rootLayout)
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
            ModAuditLogPopup::create(FetchLogsFilters {
                .target = m_data->accountId,
            })->show();
        })
        .zOrder(btnorder::History)
        .parent(m_rootMenu);

    auto& nm = NetworkManagerImpl::get();
    auto role = nm.getUserHighestRole();

    if (nm.getModPermissions().canSetPassword) {
        // Password button
        Build<CCSprite>::create("button-admin-password.png"_spr)
            .scale(btnScale)
            .intoMenuItem([this] {
                auto popup = InputPopup::create("bigFont.fnt");
                popup->setMaxCharCount(32);
                popup->setWidth(280.f);
                popup->setPasswordMode(true);
                popup->setPlaceholder("Password");
                popup->setTitle("Set Password");
                popup->setCallback([this](auto outcome) {
                    if (outcome.cancelled) return;
                    NetworkManagerImpl::get().sendAdminSetPassword(m_data->accountId, outcome.text);
                });

                popup->show();
            })
            .zOrder(btnorder::AdminPassword)
            .parent(m_rootMenu);
    }

    // kick button
    Build<CCSprite>::create("button-admin-kick.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            globed::quickPopup(
                "Confirm",
                "Are you sure you want to <cr>kick</c> this person from the server?",
                "Cancel",
                "Yes",
                [this](auto, bool yeah) {
                    if (!yeah) return;

                    auto popup = InputPopup::create("chatFont.fnt");
                    popup->setMaxCharCount(128);
                    popup->setWidth(360.f);
                    popup->setPlaceholder("Reason");
                    popup->setTitle(fmt::format("Kick {}", m_score->m_userName));
                    popup->setCallback([this](auto outcome) {
                        if (outcome.cancelled) return;
                        NetworkManagerImpl::get().sendAdminKick(m_data->accountId, outcome.text);
                    });

                    popup->show();
                }
            );
        })
        .zOrder(btnorder::Kick)
        .parent(m_rootMenu);

    // notice button
    Build<CCSprite>::create("button-admin-notice.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            auto popup = ModNoticeSetupPopup::create();
            popup->setupUser(m_data->accountId);
            popup->show();
        })
        .zOrder(btnorder::Notice)
        .parent(m_rootMenu);

    m_rootMenu->updateLayout();
}

void ModUserPopup::recreateRoleButton() {
    cue::resetNode(m_roleModifyButton);

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
            if (!NetworkManagerImpl::get().getModPermissions().canEditRoles) return;

            auto popup = ModRoleModifyPopup::create(m_data->accountId, m_data->roles);
            popup->setCallback([this] {
                this->fullRefresh();
            });
            popup->show();
        })
        .parent(m_nameLayout);

    m_nameLayout->updateLayout();
}

void ModUserPopup::createMuteAndBanButtons() {
    cue::resetNode(m_banButton);
    cue::resetNode(m_muteButton);
    cue::resetNode(m_roomBanButton);

    // Ban button
    m_banButton = Build<CCSprite>::create(m_data->activeBan ? "button-admin-unban.png"_spr : "button-admin-ban.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            this->showPunishmentPopup(UserPunishmentType::Ban);
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
            this->showPunishmentPopup(UserPunishmentType::Mute);
        })
        .zOrder(btnorder::Mute)
        .parent(m_rootMenu);

    // Room ban button
    m_roomBanButton = Build<CCSprite>::create(m_data->activeRoomBan ? "button-admin-room-unban.png"_spr : "button-admin-room-ban.png"_spr)
        .with([&](CCSprite* btn) {
            cue::rescaleToMatch(btn, expSize);
        })
        .intoMenuItem([this] {
            this->showPunishmentPopup(UserPunishmentType::RoomBan);
        })
        .zOrder(btnorder::RoomBan)
        .parent(m_rootMenu);

    m_rootMenu->updateLayout();
}

void ModUserPopup::fullRefresh() {
    this->startLoadingProfile(m_data->accountId);
}

void ModUserPopup::showPunishmentPopup(UserPunishmentType type) {
    auto& active =
        type == UserPunishmentType::Ban ? m_data->activeBan :
        type == UserPunishmentType::Mute ? m_data->activeMute :
        m_data->activeRoomBan;

    auto popup = ModPunishPopup::create(m_data->accountId, type, active);
    popup->setCallback([this] {
        this->fullRefresh();
    });
    popup->show();
}

void ModUserPopup::startLoadingProfile(int id) {
    this->startLoadingProfile(fmt::to_string(id), true);
}

void ModUserPopup::startLoadingProfile(const std::string& query, bool isId) {
    // remove all custom nodes
    cue::resetNode(m_roleModifyButton);
    cue::resetNode(m_banButton);
    cue::resetNode(m_muteButton);
    cue::resetNode(m_roomBanButton);
    cue::resetNode(m_loadCircle);
    cue::resetNode(m_nameLayout);
    cue::resetNode(m_rootMenu);

    m_data.reset();

    m_loadCircle = Build<cue::LoadingCircle>::create()
        .pos(this->center())
        .parent(m_mainLayer);

    m_loadCircle->fadeIn();
    m_query = query;
    m_queryIsId = isId;

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
    } else if (m_queryIsId) {
        queryAccountId = utils::numFromString<int>(m_query).unwrapOr(0);
    }

    // if this is a refresh, reuse the user score
    if (m_score) {
        this->onUserInfoLoaded(Ok(m_score), false);
        return;
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

void ModUserPopup::onUserInfoLoaded(geode::Result<GJUserScore*> res, bool sendUpdate) {
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
        m_data = Data{};
    }

    m_data->accountId = m_score->m_accountID;
    this->initUi();

    if (sendUpdate) {
        this->sendUpdateMessage();
    }
}

void ModUserPopup::sendUpdateMessage() {
    NetworkManagerImpl::get().sendAdminUpdateUser(
        m_data->accountId,
        m_score->m_userName,
        m_score->m_playerCube,
        m_score->m_color1,
        m_score->m_color2,
        m_score->m_glowEnabled ? (uint16_t)m_score->m_color3 : (uint16_t)-1
    );
}

void ModUserPopup::getUserInfoFinished(GJUserScore* p0) {
    this->onUserInfoLoaded(Ok(p0));
}

void ModUserPopup::getUserInfoFailed(int p0) {
    this->onUserInfoLoaded(Err("Failed to find the user.{}", p0 == 0 ? "" : fmt::format(" (error code {})", p0)));
}

void ModUserPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2) {
    if (!levels || levels->count() == 0) {
        this->onUserInfoLoaded(Err("Failed to search for user (no results)"));
    } else {
        this->onUserInfoLoaded(Ok(CCArrayExt<GJUserScore>(levels)[0]));
    }
}

void ModUserPopup::loadLevelsFailed(char const* key, int p1) {
    this->onUserInfoLoaded(Err("Failed to find the user.{}", p1 == 0 ? "" : fmt::format(" (error code {})", p1)));
}

}