#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

class RoomJoinPopup : public geode::Popup<> {
public:
    constexpr static float POPUP_WIDTH = 240.f;
    constexpr static float POPUP_HEIGHT = 140.f;

    static RoomJoinPopup* create();

protected:
    bool setup() override;

    geode::TextInput* roomIdInput;

    uint32_t attemptedJoinCode = 0;
    bool awaitingResponse = false;
};