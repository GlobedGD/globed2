#include "room_password_popup.hpp"

#include <net/network_manager.hpp>
#include <data/packets/client/room.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool RoomPasswordPopup::setup(uint32_t id) {
    this->setTitle("Join room");

    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    // room id hint
    Build<CCLabelBMFont>::create("Room Password", "bigFont.fnt")
        .scale(0.3f)
        .pos(popupCenter, POPUP_HEIGHT + 45.f)
        .parent(m_mainLayer);

    // room id input node
    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "", "chatFont.fnt", std::string(util::misc::STRING_DIGITS), 7)
        .pos(popupCenter, POPUP_HEIGHT + 25.f)
        .parent(m_mainLayer)
        .store(roomPassInput);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, id](auto) {
            NetworkManager::get().send(JoinRoomPacket::create(id, roomPassInput->getString()));
            this->onClose(nullptr);
        })
        .id("join-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("join-btn-menu"_spr)
        .pos(popupCenter, 120.f)
        .parent(m_mainLayer);

    return true;
}

RoomPasswordPopup* RoomPasswordPopup::create(uint32_t id) {
    auto ret = new RoomPasswordPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, id)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}