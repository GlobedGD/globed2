#include "admin_popup.hpp"

#include "send_notice_popup.hpp"
#include "user_popup.hpp"
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/account.hpp>
#include <managers/error_queues.hpp>
#include <net/network_manager.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdminPopup::setup() {
    this->setTitle("Globed Admin Panel");

    auto sizes = util::ui::getPopupLayout(m_size);

    auto& nm = NetworkManager::get();
    bool authorized = nm.isAuthorizedAdmin();
    if (!authorized) return false;

    nm.addListener<AdminUserDataPacket>([](auto packet) {
        AdminUserPopup::create(packet->userEntry, packet->accountData)->show();
    });

    auto* topRightCorner = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAxisReverse(true))
        .pos(sizes.topRight - CCPoint{10.f, 30.f})
        .contentSize(65.f, 0.f)
        .anchorPoint(1.f, 0.5f)
        .parent(m_mainLayer)
        .collect();

    // logout button
    Build<CCSprite>::createSpriteName("icon-logout.png"_spr)
        .scale(1.25f)
        .intoMenuItem([this](auto) {
            GlobedAccountManager::get().clearAdminPassword();
            NetworkManager::get().clearAdminStatus();
            this->onClose(this);
        })
        .parent(topRightCorner);

    // kick all button
    Build<CCSprite>::createSpriteName("icon-kick-all.png"_spr)
        .scale(1.0f)
        .intoMenuItem([this](auto) {
            geode::createQuickPopup("Confirm action", "Are you sure you want to kick <cy>all players on the server</c>?", "Cancel", "Confirm", [this](auto, bool btn2) {
                if (!btn2) return;

                AskInputPopup::create("Kick everyone", [this](auto reason) {
                    NetworkManager::get().send(AdminDisconnectPacket::create("@everyone", reason));
                }, 100, "Kick reason", util::misc::STRING_PRINTABLE_INPUT, 1.f)->show();
            });
        })
        .parent(topRightCorner);

    topRightCorner->updateLayout();

    // send notice menu
    auto* sendNoticeWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(sizes.center.width, sizes.bottom + 40.f)
        .parent(m_mainLayer)
        .collect();

    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "message", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 160)
        .parent(sendNoticeWrapper)
        .store(messageInput);

    Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            auto message = this->messageInput->getString();
            if (!message.empty()) {
                AdminSendNoticePopup::create(message)->show();
            }
        })
        .parent(sendNoticeWrapper);

    sendNoticeWrapper->updateLayout();

    // find user menu

    auto* findUserWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(sizes.center.width, sizes.center.height)
        .parent(m_mainLayer)
        .collect();

    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "user", "chatFont.fnt", std::string(util::misc::STRING_ALPHANUMERIC), 16)
        .parent(findUserWrapper)
        .store(userInput);

    Build<ButtonSprite>::create("Find", "bigFont.fnt", "GJ_button_01.png", 0.5f)
        .intoMenuItem([this](auto) {
            auto user = this->userInput->getString();
            if (!user.empty()) {
                auto& nm = NetworkManager::get();
                nm.send(AdminGetUserStatePacket::create(user));
            }
        })
        .parent(findUserWrapper);

    findUserWrapper->updateLayout();

    return true;
}

void AdminPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);

    auto& nm = NetworkManager::get();
    nm.removeListener<AdminUserDataPacket>(util::time::seconds(3));
}

AdminPopup* AdminPopup::create() {
    auto ret = new AdminPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}