#pragma once
#include <defs/all.hpp>
#include <data/packets/all.hpp>

class RoomPasswordPopup : public geode::Popup<uint32_t> {
public:
    constexpr static float POPUP_WIDTH = 240.f;
    constexpr static float POPUP_HEIGHT = 140.f;

    static RoomPasswordPopup* create(uint32_t);

protected:
    bool setup(uint32_t) override;

    geode::InputNode* roomPassInput;
};