#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

#include "room_popup.hpp"

using namespace geode::prelude;

class CreateRoomPopup : public geode::Popup<RoomPopup*> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;

    static CreateRoomPopup* create(RoomPopup* parent);

protected:
    InputNode* roomNameInput;
    InputNode* passwordInput;
    InputNode* playerLimitInput;

    bool setup(RoomPopup* parent) override;
};