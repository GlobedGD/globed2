#include "admin_popup.hpp"

#include "send_notice_popup.hpp"
#include "user_popup.hpp"
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/account.hpp>
#include <managers/error_queues.hpp>
#include <net/network_manager.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdminPopup::setup() {
    auto sizes = util::ui::getPopupLayout(m_size);

    auto& nm = NetworkManager::get();
    bool authorized = nm.isAuthorizedAdmin();
    if (!authorized) return false;

    nm.addListener<AdminSuccessMessagePacket>([](auto packet) {
        ErrorQueues::get().success(packet->message);
    });

    nm.addListener<AdminErrorPacket>([](auto packet) {
        ErrorQueues::get().warn(packet->message);
    });

    nm.addListener<AdminUserDataPacket>([](auto packet) {
        AdminUserPopup::create(packet->userEntry, packet->accountData)->show();
    });

    // logout button
    Build<CCSprite>::createSpriteName("secretDoorBtn_open_001.png")
        .intoMenuItem([this](auto) {
            GlobedAccountManager::get().clearAdminPassword();
            NetworkManager::get().clearAdminStatus();
            this->onClose(this);
        })
        .pos(sizes.topRight - CCPoint{30.f, 30.f})
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    // send notice menu
    auto* sendNoticeWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(sizes.center.width, sizes.center.height - 100.f)
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

    Build<InputNode>::create(POPUP_WIDTH * 0.5f, "user", "chatFont.fnt", std::string(util::misc::STRING_ALPHANUMERIC), 16)
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
    nm.removeListener<AdminSuccessMessagePacket>(util::time::seconds(3));
    nm.removeListener<AdminErrorPacket>(util::time::seconds(3));
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