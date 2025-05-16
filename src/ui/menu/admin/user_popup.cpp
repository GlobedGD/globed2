#include "user_popup.hpp"

#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/admin.hpp>
#include <managers/error_queues.hpp>
#include <managers/role.hpp>
#include <managers/popup.hpp>
#include <net/manager.hpp>
#include <ui/menu/admin/edit_role_popup.hpp>
#include <ui/game/chat/unread_badge.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/color_input_popup.hpp>
#include <util/math.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/gd.hpp>

#include "punish_user_popup.hpp"
#include "punishment_history_popup.hpp"

namespace btnorder {
    static constexpr int Ban = 41;
    static constexpr int Mute = 42;
    static constexpr int Whitelist = 43;
    static constexpr int History = 44;
    static constexpr int AdminPassword = 45;
    static constexpr int Kick = 46;
    static constexpr int Notice = 47;
}

class AdminUserPopup::WaitForResponsePopup : public geode::Popup<AdminUserPopup*> {
public:
    static WaitForResponsePopup* create(AdminUserPopup* popup) {
        auto ret = new WaitForResponsePopup;
        if (ret->initAnchored(160.f, 120.f, popup)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    AdminUserPopup* adminPopup;

    bool setup(AdminUserPopup* parent) {
        this->adminPopup = parent;

        Build<BetterLoadingCircle>::create()
            .parent(m_mainLayer)
            .pos(util::ui::getPopupLayoutAnchored(m_size).center)
            .with([&] (auto c) {
                c->fadeIn();
            });

        NetworkManager::get().addListener<AdminSuccessMessagePacket>(this, [this](auto pkt) {
            ErrorQueues::get().success(pkt->message);
            this->onClose(nullptr);
        }, -1000, true);

        NetworkManager::get().addListener<AdminSuccessfulUpdatePacket>(this, [this](auto pkt) {
            this->adminPopup->refreshFromUserEntry(std::move(pkt->userEntry));

            if (this->adminPopup->waitingForUpdateUsername) {
                this->adminPopup->waitingForUpdateUsername = false;

                if (this->adminPopup->queuedPacket) {
                    NetworkManager::get().send(std::move(this->adminPopup->queuedPacket));
                } else {
                    this->onClose(nullptr);
                }
            } else {
                ErrorQueues::get().success("Success");
                this->onClose(nullptr);
            }

        }, -1000, true);

        NetworkManager::get().addListener<AdminErrorPacket>(this, [this](auto pkt) {
            ErrorQueues::get().warn(pkt->message);

            this->onClose(nullptr);
        }, -1000, true);

        return true;
    }
};

using namespace geode::prelude;

bool AdminUserPopup::setup(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData) {
    this->userEntry = userEntry;
    this->accountData = accountData;

    // if we don't have their account data, request from gd servers
    if (!accountData.has_value() || accountData->name.empty()) {
        auto* glm = GameLevelManager::sharedState();
        glm->m_userInfoDelegate = this;
        glm->getGJUserInfo(userEntry.accountId);

        if (!userEntry.userName) {
            this->entryWasEmpty = true;
        }

        Build<BetterLoadingCircle>::create()
            .pos(util::ui::getPopupLayoutAnchored(m_size).center)
            .parent(m_mainLayer)
            .store(loadingCircle);

        loadingCircle->fadeIn();
    } else {
        this->onProfileLoaded();
    }

    auto& nm = NetworkManager::get();

    return true;
}

constexpr static float btnScale = 0.85f;

void AdminUserPopup::onProfileLoaded() {
    GLOBED_REQUIRE(accountData.has_value(), "no account data umm")

    auto sizes = util::ui::getPopupLayoutAnchored(m_size);

    auto& data = accountData.value();

    // name layout
    Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(sizes.centerTop - CCPoint{0.f, 20.f})
        .parent(m_mainLayer)
        .store(nameLayout);

    // name label
    Build<CCLabelBMFont>::create(data.name.c_str(), "bigFont.fnt")
        .limitLabelWidth(160.f, 0.8f, 0.1f)
        .intoMenuItem([this] {
            auto& data = accountData.value();
            util::gd::openProfile(data.accountId, data.userId, data.name);
        })
        .parent(nameLayout)
        .collect();

    // // color palette button
    // Build<ColorChannelSprite>::create()
    //     .scale(0.6f)
    //     .color(this->getCurrentNameColor())
    //     .store(nameColorSprite)
    //     .intoMenuItem([this](auto) {
    //         GlobedColorInputPopup::create(this->getCurrentNameColor(), [this](auto color) {
    //             // if our role doesn't permit editing the color, don't do anything
    //             if (!AdminManager::get().getRole().editRole) {
    //                 this->onColorSelected(color);
    //             }
    //         })->show();
    //     })
    //     .parent(nameLayout);

    // role modify button
    this->recreateRoleModifyButton();

    nameLayout->updateLayout();

    auto* rootLayout = Build<CCNode>::create()
        .pos(sizes.fromCenter(0.f, -5.f))
        .anchorPoint(0.5f, 0.5f)
        .contentSize(sizes.popupSize.width * 0.8f, sizes.popupSize.height * 0.6f)
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

    Build<CCMenu>::create()
        .ignoreAnchorPointForPos(false)
        .contentSize(rootLayout->getScaledContentSize() * 0.95f)
        .anchorPoint(0.5f, 0.5f)
        .pos(rootLayout->getScaledContentSize() / 2.f)
        .parent(rootLayout)
        .layout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAutoScale(false)
                    ->setGrowCrossAxis(true))
        .store(rootMenu);

    this->createBanAndMuteButtons();

    // Whitelist button
    Build<CCSprite>::createSpriteName(userEntry.isWhitelisted ? "button-admin-unwhitelist.png"_spr : "button-admin-whitelist.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this](CCMenuItemSpriteExtra* btn) {
            geode::createQuickPopup(
                "Confirm",
                userEntry.isWhitelisted ?
                    "Are you sure you want to remove this person from the whitelist?"
                    : "Are you sure you want to whitelist this person?",
                "Cancel",
                "Yes",
                [this, btn](auto, bool confirm) {
                    if (!confirm) return;

                    auto packet = AdminWhitelistPacket::create(userEntry.accountId, !userEntry.isWhitelisted);

                    this->performAction(std::move(packet));

                    userEntry.isWhitelisted = !userEntry.isWhitelisted;

                    btn->setSprite(
                        Build<CCSprite>::createSpriteName(
                            userEntry.isWhitelisted ? "button-admin-unwhitelist.png"_spr : "button-admin-whitelist.png"_spr
                            )
                            .scale(btnScale)
                            .collect()
                    );
                }
            );
        })
        .zOrder(btnorder::Whitelist)
        .parent(rootMenu);

    // History button
    Build<CCSprite>::createSpriteName("button-admin-history.png"_spr)
        .scale(btnScale)
        .with([&](auto* btn) {
            // if the user has past punishments, show a little badge with the count
            if (userEntry.punishmentCount > 0) {
                Build<UnreadMessagesBadge>::create(userEntry.punishmentCount)
                    .pos(btn->getScaledContentSize() + CCPoint{1.f, 1.f})
                    .scale(0.7f)
                    .id("count-icon"_spr)
                    .parent(btn)
                    ;
            }
        })
        .intoMenuItem([this] {
            AdminPunishmentHistoryPopup::create(userEntry.accountId)->show();
        })
        .zOrder(btnorder::History)
        .parent(rootMenu);

    if (AdminManager::get().getRole().admin) {
        // Password button
        Build<CCSprite>::createSpriteName("button-admin-password.png"_spr)
            .scale(btnScale)
            .intoMenuItem([this] {
                AskInputPopup::create("Change Password", [this, accountId = accountData->accountId](auto pwd) {
                    auto packet = AdminSetAdminPasswordPacket::create(accountId, util::format::trim(pwd));

                    this->performAction(std::move(packet));
                }, 64, "Password", util::misc::STRING_PRINTABLE_INPUT, 0.85f)->show();
            })
            .zOrder(btnorder::AdminPassword)
            .parent(rootMenu);
    }

    // kick button
    Build<CCSprite>::createSpriteName("button-admin-kick.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AskInputPopup::create(fmt::format("Kick {}", accountData->name), [this, accountId = accountData->accountId](auto reason) {
                auto packet = AdminDisconnectPacket::create(std::to_string(accountId), reason);
                NetworkManager::get().send(packet);

                this->showLoadingPopup();
            }, 120, "Kick reason", util::misc::STRING_PRINTABLE_INPUT, 0.6f)->show();
        })
        .zOrder(btnorder::Kick)
        .parent(rootMenu);

    // notice button
    Build<CCSprite>::createSpriteName("button-admin-notice.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AskInputPopup::create(fmt::format("Send notice to {}", accountData->name), [this, accountId = accountData->accountId](auto message, bool canReply) {
                auto msg = util::format::trim(message);
                if (msg.empty()) {
                    ErrorQueues::get().warn("Cannot send an empty notice");
                    return;
                }

                auto packet = AdminSendNoticePacket::create(AdminSendNoticeType::Person, 0, 0, std::to_string(accountId), msg, canReply, false);
                NetworkManager::get().send(packet);

                this->showLoadingPopup();
            }, 160, "Message", util::misc::STRING_PRINTABLE_INPUT, 0.6f, "User can reply?")->show();
        })
        .zOrder(btnorder::Notice)
        .parent(rootMenu);

    rootMenu->updateLayout();
}

void AdminUserPopup::onColorSelected(ccColor3B color) {
    nameColorSprite->setColor(color);
    userEntry.nameColor = util::format::colorToHex(color);
}

void AdminUserPopup::recreateRoleModifyButton() {
    if (roleModifyButton) {
        roleModifyButton->removeFromParent();
        roleModifyButton = nullptr;
    }

    CCSprite* badgeIcon = nullptr;
    if (accountData) {
        badgeIcon = util::ui::createBadgeIfSpecial(userEntry);
    }

    // make it the normal user icon
    if (!badgeIcon) {
        badgeIcon = CCSprite::createWithSpriteFrameName("role-user.png"_spr);
    }

    util::ui::rescaleToMatch(badgeIcon, util::ui::BADGE_SIZE * 1.2f);

    roleModifyButton = Build(badgeIcon)
        .intoMenuItem([this](auto) {
            if (!AdminManager::get().getRole().editRole) return;

            AdminEditRolePopup::create(this, userEntry.accountId, userEntry.userRoles)->show();
        })
        .parent(nameLayout)
        .collect();

    nameLayout->updateLayout();
}

void AdminUserPopup::createBanAndMuteButtons() {
    if (banButton) {
        banButton->removeFromParent();
    }

    if (muteButton) {
        muteButton->removeFromParent();
    }

    // Ban button
    Build<CCSprite>::createSpriteName(userEntry.activeBan ? "button-admin-unban.png"_spr : "button-admin-ban.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AdminPunishUserPopup::create(this, userEntry.accountId, true, userEntry.activeBan)->show();
        })
        .zOrder(btnorder::Ban)
        .parent(rootMenu)
        .store(banButton);

    // Mute button
    Build<CCSprite>::createSpriteName(userEntry.activeMute ? "button-admin-unmute.png"_spr : "button-admin-mute.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AdminPunishUserPopup::create(this, userEntry.accountId, false, userEntry.activeMute)->show();
        })
        .zOrder(btnorder::Mute)
        .parent(rootMenu)
        .store(muteButton);

    rootMenu->updateLayout();
}

cocos2d::ccColor3B AdminUserPopup::getCurrentNameColor() {
    auto colorRes = util::format::parseColor(userEntry.nameColor.value_or("#ffffff"));
    auto color = colorRes.unwrapOr(ccc3(255, 255, 255));
    return color;
}

void AdminUserPopup::sendUpdateUser() {
    // some validation
    if (userEntry.nameColor.has_value() && userEntry.nameColor->size() < 6) {
        userEntry.nameColor = std::nullopt;
    }

    if (!userEntry.userName.has_value()) {
        userEntry.userName = accountData->name;
    }
}

void AdminUserPopup::refreshFromUserEntry(UserEntry entry) {
    this->userEntry = std::move(entry);
    this->recreateRoleModifyButton();
    this->createBanAndMuteButtons();
}

void AdminUserPopup::performAction(std::shared_ptr<Packet> packet) {
    if (entryWasEmpty) {
        // send update username packet, put this one on hold..
        queuedPacket = std::move(packet);
        std::string username;
        if (accountData) {
            username = accountData->name;
        } else {
            username = userEntry.userName.value_or("");
        }

        if (username.empty()) {
            ErrorQueues::get().warn("Username is empty, cannot send update packet");
            NetworkManager::get().send(std::move(queuedPacket));
            entryWasEmpty = false;
            return;
        }

        entryWasEmpty = false;
        waitingForUpdateUsername = true;

        NetworkManager::get().send(AdminUpdateUsernamePacket::create(userEntry.accountId, username));
    } else {
        NetworkManager::get().send(packet);
    }

    this->showLoadingPopup();
}

void AdminUserPopup::removeLoadingCircle() {
    loadingCircle->fadeOut();
}

void AdminUserPopup::showLoadingPopup() {
    WaitForResponsePopup::create(this)->show();
}

void AdminUserPopup::getUserInfoFinished(GJUserScore* score) {
    GameLevelManager::sharedState()->m_userInfoDelegate = nullptr;
    this->removeLoadingCircle();

    userScore = score;
    accountData = PlayerRoomPreviewAccountData(
        score->m_accountID,
        score->m_userID,
        score->m_userName,
        PlayerIconDataSimple(score),
        0,
        {}
    );

    userEntry.userName = score->m_userName;

    this->onProfileLoaded();
}

void AdminUserPopup::getUserInfoFailed(int p0) {
    GameLevelManager::sharedState()->m_userInfoDelegate = nullptr;
    this->removeLoadingCircle();

    PopupManager::get().alert("Error", "Failed to fetch user data.").showInstant();
    this->onClose(this);
}

void AdminUserPopup::userInfoChanged(GJUserScore* p0) {}

void AdminUserPopup::onClose(CCObject* sender) {
    GameLevelManager::sharedState()->m_userInfoDelegate = nullptr;

    auto& nm = NetworkManager::get();

    Popup::onClose(sender);
}

AdminUserPopup* AdminUserPopup::create(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData) {
    auto ret = new AdminUserPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, userEntry, accountData)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}