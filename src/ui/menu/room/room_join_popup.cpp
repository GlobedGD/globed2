#include "room_join_popup.hpp"

#include "room_password_popup.hpp"
#include <data/packets/client/room.hpp>
#include <data/packets/server/room.hpp>
#include <managers/error_queues.hpp>
#include <net/manager.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool RoomJoinPopup::setup() {
    this->setTitle("Join room");

    auto& nm = NetworkManager::get();

    if (!nm.established()) {
        return false;
    }

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // room id hint
    Build<CCLabelBMFont>::create("Room ID", "bigFont.fnt")
        .scale(0.3f)
        .pos(rlayout.fromTop(45.f))
        .parent(m_mainLayer)
        .id("join-room-id-hint"_spr);

    // room id input node
    Build<TextInput>::create(POPUP_WIDTH * 0.75f, "", "chatFont.fnt")
        .pos(rlayout.fromTop(65.f))
        .parent(m_mainLayer)
        .id("join-room-id"_spr)
        .store(roomIdInput);

    roomIdInput->setCommonFilter(CommonFilter::Uint);
    roomIdInput->setMaxCharCount(7);

    Build<ButtonSprite>::create("Join", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            if (this->awaitingResponse) return;

            std::string codestr = this->roomIdInput->getString();
            uint32_t code = util::format::parse<uint32_t>(codestr).value_or(0);

            if (code < 100000 || code > 999999) {
                Notification::create("Invalid room ID", NotificationIcon::Warning)->show();
                return;
            }

            auto& nm = NetworkManager::get();
            if (!nm.established()) {
                return this->onClose(nullptr);
            }

            // test packet to check if pass needed (or just joining the room)
            NetworkManager::get().send(JoinRoomPacket::create(code, ""));

            this->awaitingResponse = true;
            this->attemptedJoinCode = code;
        })
        .id("join-btn"_spr)
        .intoNewParent(CCMenu::create())
        .id("join-btn-menu"_spr)
        .pos(rlayout.fromBottom(30.f))
        .parent(m_mainLayer);

    nm.addListener<RoomJoinFailedPacket>(this, [this](std::shared_ptr<RoomJoinFailedPacket> packet) {
        this->awaitingResponse = false;

        if (packet->wasProtected) {
            RoomPasswordPopup::create(this->attemptedJoinCode)->show();
            this->onClose(nullptr);
        } else {
            std::string reason = "N/A";
            if (packet->wasInvalid) reason = "Room doesn't exist";
            if (packet->wasFull) reason = "Room is full";

            ErrorQueues::get().error(fmt::format("Failed to join room: {}", reason));
        }
    }, -10, true);

    nm.addListener<RoomJoinedPacket>(this, [this](std::shared_ptr<RoomJoinedPacket> packet) {
        this->awaitingResponse = false;

        this->onClose(nullptr);
    });

    return true;
}

RoomJoinPopup* RoomJoinPopup::create() {
    auto ret = new RoomJoinPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}