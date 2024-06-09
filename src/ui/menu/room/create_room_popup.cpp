#include "create_room_popup.hpp"

#include <net/manager.hpp>
#include <data/packets/client/room.hpp>

bool CreateRoomPopup::setup(RoomPopup* parent) {
    this->setTitle("Create Room", "goldFont.fnt", 1.0f);

    float popupCenter = CCDirector::get()->getWinSize().width / 2;

    // room name hint
    Build<CCLabelBMFont>::create("Room Name", "bigFont.fnt")
        .scale(0.5f)
        .pos((popupCenter * 0.75f) - 10.f, (POPUP_HEIGHT * 0.75f) + 25.f)
        .parent(m_mainLayer);

    // room name input node
    Build<InputNode>::create(POPUP_WIDTH * 0.50f, "", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 16)
        .pos((popupCenter * 0.75f) - 10.f, (POPUP_HEIGHT * 0.75f))
        .parent(m_mainLayer)
        .store(roomNameInput);

    // Password hint
    Build<CCLabelBMFont>::create("Password", "bigFont.fnt")
        .scale(0.35f)
        .pos((popupCenter / 2.f) + 9.f, (POPUP_HEIGHT * 0.55f) + 25.f)
        .parent(m_mainLayer);

    // Password input node
    Build<InputNode>::create(POPUP_WIDTH * 0.25f, "", "chatFont.fnt", std::string(util::misc::STRING_ALPHABET), 16)
        .pos((popupCenter / 2.f) + 9.f, (POPUP_HEIGHT * 0.55f))
        .parent(m_mainLayer)
        .store(passwordInput);

    // Player Limit hint
    Build<CCLabelBMFont>::create("Player Limit", "bigFont.fnt")
        .scale(0.35f)
        .pos((popupCenter) - 26.f, (POPUP_HEIGHT * 0.55f) + 25.f)
        .parent(m_mainLayer);

    // Player Limit input node
    Build<InputNode>::create(POPUP_WIDTH * 0.25f, "", "chatFont.fnt", std::string(util::misc::STRING_DIGITS), 2)
        .pos((popupCenter) - 26.f, (POPUP_HEIGHT * 0.55f))
        .parent(m_mainLayer)
        .store(playerLimitInput);

    Build<ButtonSprite>::create("Create", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this, parent](auto) {
            std::string roomName = std::string(GJAccountManager::get()->m_username) + "'s room";
            if (roomNameInput->getString().size() != 0) roomName = roomNameInput->getString();
            uint32_t playercount = 32;
            if (playerLimitInput->getString() != "") playercount = std::stoi(playerLimitInput->getString());
            NetworkManager::get().send(CreateRoomPacket::create(roomName, passwordInput->getString(), RoomSettings {{}, playercount}));
            parent->reloadPlayerList(false);
            this->onClose(nullptr);
        })
        .intoNewParent(CCMenu::create())
        .pos(popupCenter, 60.f)
        .parent(m_mainLayer);

    return true;
}

CreateRoomPopup* CreateRoomPopup::create(RoomPopup* parent) {
    auto ret = new CreateRoomPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, parent)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}