#include "admin_popup.hpp"

#include "send_notice_popup.hpp"
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/error_queues.hpp>
#include <net/network_manager.hpp>
#include <util/misc.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool AdminPopup::setup() {
    auto winSize = CCDirector::get()->getWinSize();
    auto center = winSize / 2;

    // i hate cocos ui
    CCSize bottom{center.width, center.height - m_size.height / 2};
    CCSize left{center.width - m_size.width / 2, center.height};
    CCSize right{center.width + m_size.width / 2, center.height};

    auto& nm = NetworkManager::get();
    bool authorized = nm.isAuthorizedAdmin();
    if (!authorized) return false;

    nm.addListener<AdminSuccessMessagePacket>([](auto packet) {
        ErrorQueues::get().success(packet->message);
    });

    nm.addListener<AdminErrorPacket>([](auto packet) {
        ErrorQueues::get().warn(packet->message);
    });

    // send notice menu
    auto* sendNoticeWrapper = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(center.width, center.height - 100.f)
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

    return true;
}

void AdminPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);

    auto& nm = NetworkManager::get();
    nm.removeListener<AdminSuccessMessagePacket>();
    nm.removeListener<AdminErrorPacket>();

    nm.suppressUnhandledFor<AdminSuccessMessagePacket>(util::time::seconds(3));
    nm.suppressUnhandledFor<AdminErrorPacket>(util::time::seconds(3));
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