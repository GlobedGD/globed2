#include "room_join_popup.hpp"

#include <net/network_manager.hpp>
#include <data/packets/client/general.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool RoomJoinPopup::setup() {
    this->setTitle("Join room");

    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    // room id hint
    Build<CCLabelBMFont>::create("Room ID", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT + 45.f)
        .parent(m_mainLayer)
        .id("join-room-id-hint"_spr);

    // room id input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "", "chatFont.fnt", std::string(util::misc::STRING_DIGITS), 7)
        .pos(popupCenter, POPUP_HEIGHT + 25.f)
        .parent(m_mainLayer)
        .id("join-room-id"_spr)
        .store(roomIdInput);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            std::string codestr = this->roomIdInput->getString();
            uint32_t code = util::format::parse<uint32_t>(codestr).value_or(0);

            if (code < 100000 || code > 999999) {
                Notification::create("Invalid room ID", NotificationIcon::Warning)->show();
                return;
            }

            NetworkManager::get().send(JoinRoomPacket::create(code));
            this->onClose(nullptr);
        })
        .id("join-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("join-btn-menu"_spr)
        .pos(popupCenter, 120.f)
        .parent(m_mainLayer);

    return true;
}

RoomJoinPopup* RoomJoinPopup::create() {
    auto ret = new RoomJoinPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}