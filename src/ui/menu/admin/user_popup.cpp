#include "user_popup.hpp"

#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/admin.hpp>
#include <managers/error_queues.hpp>
#include <managers/role.hpp>
#include <net/manager.hpp>
#include <ui/menu/admin/edit_role_popup.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/color_input_popup.hpp>
#include <ui/general/slider_wrapper.hpp>
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

        NetworkManager::get().addListener<AdminSuccessfulPunishmentPacket>(this, [this](auto pkt) {
            ErrorQueues::get().success("Success");
            if (pkt->punishment.type == PunishmentType::Ban) {
                this->adminPopup->userEntry.activeBan = std::move(pkt->punishment);
            } else {
                this->adminPopup->userEntry.activeMute = std::move(pkt->punishment);
            }

            this->onClose(nullptr);
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

void AdminUserPopup::onProfileLoaded() {
    GLOBED_REQUIRE(accountData.has_value(), "no account data umm")

    auto sizes = util::ui::getPopupLayout(m_size);

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

    auto* rootMenu = Build<CCMenu>::create()
        .ignoreAnchorPointForPos(false)
        .contentSize(rootLayout->getScaledContentSize() * 0.95f)
        .anchorPoint(0.5f, 0.5f)
        .pos(rootLayout->getScaledContentSize() / 2.f)
        .parent(rootLayout)
        .layout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAutoScale(false)
                    ->setGrowCrossAxis(true))
        .collect();

    constexpr static float btnScale = 0.85f;

    // Ban button
    Build<CCSprite>::createSpriteName(userEntry.activeBan ? "button-admin-unban.png"_spr : "button-admin-ban.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AdminPunishUserPopup::create(userEntry.accountId, true)->show();
        })
        .zOrder(btnorder::Ban)
        .parent(rootMenu);

    // Mute button
    Build<CCSprite>::createSpriteName(userEntry.activeMute ? "button-admin-unmute.png"_spr : "button-admin-mute.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AdminPunishUserPopup::create(userEntry.accountId, false)->show();
        })
        .zOrder(btnorder::Mute)
        .parent(rootMenu);

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
                    NetworkManager::get().send(packet);

                    WaitForResponsePopup::create(this)->show();

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
                    NetworkManager::get().send(packet);

                    WaitForResponsePopup::create(this)->show();
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

                WaitForResponsePopup::create(this)->show();
            }, 120, "Kick reason", util::misc::STRING_PRINTABLE_INPUT, 0.6f)->show();
        })
        .zOrder(btnorder::Kick)
        .parent(rootMenu);

    // notice button
    Build<CCSprite>::createSpriteName("button-admin-notice.png"_spr)
        .scale(btnScale)
        .intoMenuItem([this] {
            AskInputPopup::create(fmt::format("Send notice to {}", accountData->name), [this, accountId = accountData->accountId](auto message) {
                auto packet = AdminSendNoticePacket::create(AdminSendNoticeType::Person, 0, 0, std::to_string(accountId), message);
                NetworkManager::get().send(packet);

                WaitForResponsePopup::create(this)->show();
            }, 160, "Message", util::misc::STRING_PRINTABLE_INPUT, 0.6f)->show();
        })
        .zOrder(btnorder::Notice)
        .parent(rootMenu);

    rootMenu->updateLayout();

    // // root column layout for all the next stuff
    // auto* rootLayout = Build<CCNode>::create()
    //     .pos(sizes.center)
    //     .anchorPoint(0.5f, 0.5f)
    //     .contentSize(sizes.popupSize)
    //     .layout(
    //         ColumnLayout::create()
    //                 ->setGap(10.f)
    //                 ->setAxisReverse(true)
    //                 ->setAutoScale(false)
    //     )
    //     .id("root-layout"_spr)
    //     .parent(m_mainLayer)
    //     .collect();

    // // ban, mute, whitelist buttons and duration slider
    // auto* violationsLayout = Build<CCMenu>::create()
    //     .layout(RowLayout::create()->setGap(5.f))
    //     .id("violations-layout"_spr)
    //     .parent(rootLayout)
    //     .collect();

    // constexpr float buttonScale = 0.9f;

    // auto banUnbanned = Build<CCSprite>::createSpriteName("icon-ban.png"_spr)
    //     .scale(buttonScale);
    // auto banBanned = Build<CCSprite>::createSpriteName("icon-unban.png"_spr)
    //     .scale(buttonScale);
    // auto* btnBanned = Build<CCMenuItemToggler>(CCMenuItemToggler::create(banUnbanned, banBanned, this, menu_selector(AdminUserPopup::onViolationChanged)))
    //     .tag(TAG_BAN)
    //     .parent(violationsLayout)
    //     .collect();
    // btnBanned->toggle(userEntry.activeBan.has_value());

    // auto muteUnmuted = Build<CCSprite>::createSpriteName("icon-mute.png"_spr)
    //     .scale(buttonScale);
    // auto muteMuted = Build<CCSprite>::createSpriteName("icon-unmute.png"_spr)
    //     .scale(buttonScale);
    // auto* btnMuted = Build<CCMenuItemToggler>(CCMenuItemToggler::create(muteUnmuted, muteMuted, this, menu_selector(AdminUserPopup::onViolationChanged)))
    //     .tag(TAG_MUTE)
    //     .parent(violationsLayout)
    //     .collect();
    // btnMuted->toggle(userEntry.activeMute.has_value());

    // auto whitelistOff = Build<CCSprite>::createSpriteName("icon-whitelist.png"_spr)
    //     .scale(buttonScale);
    // auto whitelistOn = Build<CCSprite>::createSpriteName("icon-unwhitelist.png"_spr)
    //     .scale(buttonScale);
    // auto* btnWhitelist = Build<CCMenuItemToggler>(CCMenuItemToggler::create(whitelistOff, whitelistOn, this, menu_selector(AdminUserPopup::onViolationChanged)))
    //     .tag(TAG_WHITELIST)
    //     .parent(violationsLayout)
    //     .collect();
    // btnWhitelist->toggle(userEntry.isWhitelisted);

    // // kick button
    // Build<CCSprite>::createSpriteName("icon-kick.png"_spr)
    //     .scale(0.8f)
    //     .intoMenuItem([this](auto) {
    //         AskInputPopup::create(fmt::format("Kick {}", accountData->name), [accountId = accountData->accountId](auto reason) {
    //             auto packet = AdminDisconnectPacket::create(std::to_string(accountId), reason);
    //             NetworkManager::get().send(packet);
    //         }, 120, "Kick reason", util::misc::STRING_PRINTABLE_INPUT, 0.6f)->show();
    //     })
    //     .parent(violationsLayout);

    // // notice send button
    // Build<CCSprite>::createSpriteName("GJ_chatBtn_001.png")
    //     .scale(0.8f)
    //     .intoMenuItem([this](auto) {
    //         AskInputPopup::create(fmt::format("Send notice to {}", accountData->name), [accountId = accountData->accountId](auto message) {
    //             auto packet = AdminSendNoticePacket::create(AdminSendNoticeType::Person, 0, 0, std::to_string(accountId), message);
    //             NetworkManager::get().send(packet);
    //         }, 160, "Message", util::misc::STRING_PRINTABLE_INPUT, 0.6f)->show();
    //     })
    //     .parent(violationsLayout);

    // auto* slider = Build<SliderWrapper>::create(
    //         Slider::create(this, menu_selector(AdminUserPopup::onViolationDurationChanged), 0.5f)
    //     )
    //     .parent(rootLayout)
    //     .collect();

    // slider->slider->setValue(0.f);

    // Build<CCLabelBMFont>::create("", "goldFont.fnt")
    //     .scale(0.4f)
    //     .pos(slider->getScaledContentSize() / 2 + CCPoint{0.f, 10.f})
    //     .parent(slider)
    //     .store(banDurationText);

    // // if (userEntry.violationExpiry.has_value()) {
    // //     uint64_t expiry = userEntry.violationExpiry.value();

    // //     slider->slider->setValue(floatFromExpiry(expiry));

    // //     auto expiryString = fmt::format("Expires: in {:.1f} days", floatFromExpiryDays(expiry));
    // //     banDurationText->setString(expiryString.c_str());
    // // }

    // // violation reason
    // Build<TextInput>::create(m_size.width * 0.8f, "reason", "chatFont.fnt")
    //     .parent(rootLayout)
    //     .store(inputReason);

    // // inputReason->setCommonFilter(CommonFilter::Any);
    // // inputReason->setMaxCharCount(160);
    // // inputReason->setCallback([this](auto reason) {
    // //     if (reason.empty()) {
    // //         userEntry.violationReason = std::nullopt;
    // //     } else {
    // //         userEntry.violationReason = reason;
    // //     }
    // // });

    // // if (userEntry.violationReason.has_value()) {
    // //     inputReason->setString(userEntry.violationReason.value());
    // // }

    // // admin password field
    // if (AdminManager::get().getRole().admin) {
    //     auto* layout = Build<CCMenu>::create()
    //         .pos(0.f, 0.f)
    //         .layout(RowLayout::create())
    //         .parent(rootLayout)
    //         .collect();

    //     Build<TextInput>::create(m_size.width * 0.8f, "admin password", "bigFont.fnt")
    //         .parent(layout)
    //         .store(inputAdminPassword);

    //     // inputAdminPassword->setPasswordMode(true);
    //     // inputAdminPassword->setMaxCharCount(32);
    //     // inputAdminPassword->setFilter(std::string(util::misc::STRING_PRINTABLE_INPUT));

    //     // inputAdminPassword->setString(userEntry.adminPassword.value_or(""));
    //     // inputAdminPassword->setCallback([this](auto password) {
    //     //     this->userEntry.adminPassword = password;
    //     // });

    //     layout->updateLayout();
    // }

    // // banDurationText->setVisible(userEntry.isBanned || userEntry.isMuted);

    // violationsLayout->updateLayout();

    // rootLayout->updateLayout();

    // // apply button
    // Build<ButtonSprite>::create("Update", "goldFont.fnt", "GJ_button_01.png", 1.0f)
    //     .scale(1.f)
    //     .intoMenuItem([this](auto) {
    //         this->sendUpdateUser();
    //     })
    //     .pos(sizes.centerBottom + CCPoint{0.f, 25.f})
    //     .intoNewParent(CCMenu::create())
    //     .pos(0.f, 0.f)
    //     .parent(m_mainLayer);
}

void AdminUserPopup::onColorSelected(ccColor3B color) {
    nameColorSprite->setColor(color);
    userEntry.nameColor = util::format::colorToHex(color);
}

void AdminUserPopup::onViolationChanged(cocos2d::CCObject* sender) {
    // int tag = sender->getTag();

    // bool* destination = nullptr;
    // switch (tag) {
    //     case TAG_BAN: destination = &userEntry.isBanned; break;
    //     case TAG_MUTE: destination = &userEntry.isMuted; break;
    //     case TAG_WHITELIST: destination = &userEntry.isWhitelisted; break;
    // }


    // GLOBED_REQUIRE(destination != nullptr, "invalid tag in AdminUserPopup::onViolationChanged")

    // *destination = !static_cast<CCMenuItemToggler*>(sender)->isOn();

    // // if unmuted and unbanned, hide the text label
    // banDurationText->setVisible(userEntry.isBanned || userEntry.isMuted);
}

void AdminUserPopup::onViolationDurationChanged(cocos2d::CCObject* sender) {
    // float value = static_cast<SliderThumb*>(sender)->getValue();

    // if (util::math::equal(value, 0.f)) {
    //     userEntry.violationExpiry = std::nullopt;
    //     banDurationText->setString("Expires: never");
    // } else {
    //     userEntry.violationExpiry = expiryFromFloatUnix(value);

    //     banDurationText->setString(fmt::format("Expires: in {:.1f} days", expiryFromFloat(value)).c_str());
    // }
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

            AdminEditRolePopup::create(userEntry.userRoles, [this](const std::vector<std::string>& roles) {
                userEntry.userRoles = roles;
                this->recreateRoleModifyButton();
            })->show();
        })
        .parent(nameLayout)
        .collect();

    nameLayout->updateLayout();
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

    // auto& nm = NetworkManager::get();
    // nm.send(AdminUpdateUserPacket::create(this->userEntry));
}

void AdminUserPopup::removeLoadingCircle() {
    loadingCircle->fadeOut();
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

    FLAlertLayer::create("Error", "Failed to fetch user data.", "Ok")->show();
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
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, userEntry, accountData)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}