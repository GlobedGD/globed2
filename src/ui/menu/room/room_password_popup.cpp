#include "room_password_popup.hpp"

#include <net/manager.hpp>
#include <data/packets/client/room.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool RoomPasswordPopup::setup(uint32_t id) {
    this->setTitle("Join room");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    this->setID("room-password-popup"_spr);

    // room password hint
    Build<CCLabelBMFont>::create("Room Password", "bigFont.fnt")
        .scale(0.3f)
        .pos(rlayout.fromCenter(0.f, 30.f))
        .parent(m_mainLayer);

    // room password input node
    Build<TextInput>::create(POPUP_WIDTH * 0.75f, "", "chatFont.fnt")
        .pos(rlayout.fromCenter(0.f, 10.f))
        .parent(m_mainLayer)
        .store(roomPassInput);

    roomPassInput->setFilter(std::string(util::misc::STRING_ALPHANUMERIC));
    roomPassInput->setMaxCharCount(16);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, id](auto) {
            NetworkManager::get().send(JoinRoomPacket::create(id, roomPassInput->getString()));
            this->onClose(nullptr);
        })
        .id("join-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("join-btn-menu"_spr)
        .pos(rlayout.fromBottom(25.f))
        .parent(m_mainLayer);

    return true;
}

RoomPasswordPopup* RoomPasswordPopup::create(uint32_t id) {
    auto ret = new RoomPasswordPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, id)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}