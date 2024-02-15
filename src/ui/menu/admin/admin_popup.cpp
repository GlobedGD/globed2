#include "admin_popup.hpp"

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

    bool authorized = NetworkManager::get().isAuthorizedAdmin();
    if (!authorized) {
        Build<InputNode>::create(POPUP_WIDTH * 0.75f, "password", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 32)
            .pos(center.width, center.height + 30.f)
            .parent(m_mainLayer)
            .store(passwordInput);

        Build<ButtonSprite>::create("Login", "bigFont.fnt", "GJ_button_01.png", 0.8f)
            .intoMenuItem([this](auto) {
                NetworkManager::get().send(AdminAuthPacket::create(this->passwordInput->getString()));
                this->onClose(this);
            })
            .pos(center.width, center.height - 20.f)
            .intoNewParent(CCMenu::create())
            .pos(0.f, 0.f)
            .parent(m_mainLayer);
    } else {
        // actual admin ui

        auto dcMenu = Build<CCMenu>::create()
            .layout(RowLayout::create()
                        ->setGap(3.f))
            .anchorPoint(0.5f, 0.f)
            .pos(center.width, center.height)
            .parent(m_mainLayer)
            .id("admin-dc-menu"_spr)
            .collect();

        // user disconnect input box
        Build<InputNode>::create(m_size.width * 0.25f, "user", "chatFont.fnt", std::string(util::misc::STRING_ALPHANUMERIC) + "@", 16)
            .parent(dcMenu)
            .id("admin-dc-input"_spr)
            .store(disconnectUserInput);

        // disconnect button
        Build<ButtonSprite>::create("Kick", "bigFont.fnt", "GJ_button_01.png", 0.5f)
            .intoMenuItem([this](auto) {
                auto user = this->disconnectUserInput->getString();
                auto message = this->messageInput->getString();

                auto packet = AdminDisconnectPacket::create(user, message);
                NetworkManager::get().send(packet);
            })
            .parent(dcMenu);

        dcMenu->updateLayout();

        auto menu = Build<CCMenu>::create()
            .pos(0.f, 0.f)
            .parent(m_mainLayer)
            .id("admin-buttons-menu"_spr)
            .collect();

        // message input box
        Build<InputNode>::create(m_size.width * 0.75f, "message", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 160)
            .pos(center.width, center.height + 60.f)
            .parent(m_mainLayer)
            .id("admin-message-input"_spr)
            .store(messageInput);

        // send to everyone
        Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
            .intoMenuItem([this](auto) {
                this->commonSend(AdminSendNoticeType::Everyone);
            })
            .pos(bottom.width, bottom.height + 30.f)
            .parent(menu);

        // send to a specific account ID or username
        auto sendAccId = Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
            .intoMenuItem([this](auto) {
                this->commonSend(AdminSendNoticeType::Person);
            })
            .pos(left.width + 40.f, bottom.height + 30.f)
            .parent(menu)
            .collect();

        Build<InputNode>::create(sendAccId->getScaledContentSize().width, "player", "chatFont.fnt", std::string(util::misc::STRING_ALPHANUMERIC), 16)
            .pos(left.width + 40.f, bottom.height + 70.f)
            .parent(m_mainLayer)
            .store(playerInput);

        // send to a specific room/level
        auto sendRoomLevel = Build<ButtonSprite>::create("Send", "bigFont.fnt", "GJ_button_01.png", 0.5f)
            .intoMenuItem([this](auto) {
                this->commonSend(AdminSendNoticeType::RoomOrLevel);
            })
            .pos(right.width - 40.f, bottom.height + 30.f)
            .parent(menu)
            .collect();

        Build<InputNode>::create(sendRoomLevel->getScaledContentSize().width, "level ID", "chatFont.fnt", std::string(util::misc::STRING_DIGITS), 10)
            .pos(right.width - 40.f, bottom.height + 70.f)
            .parent(m_mainLayer)
            .store(levelIdInput);

        Build<InputNode>::create(sendRoomLevel->getScaledContentSize().width, "room ID", "chatFont.fnt", std::string(util::misc::STRING_DIGITS), 10)
            .pos(right.width - 40.f, bottom.height + 105.f)
            .parent(m_mainLayer)
            .store(roomIdInput);
    }

    return true;
}

void AdminPopup::commonSend(AdminSendNoticeType type) {
    if (messageInput->getString().empty()) return;

    uint32_t roomId = 0;
    int levelId = 0;

    if (type == AdminSendNoticeType::RoomOrLevel) {
        levelId = util::format::parse<int>(levelIdInput->getString()).value_or(0);
        roomId = util::format::parse<uint32_t>(roomIdInput->getString()).value_or(0);
    }

    auto packet = AdminSendNoticePacket::create(type, roomId, levelId, playerInput->getString(), messageInput->getString());
    NetworkManager::get().send(packet);
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