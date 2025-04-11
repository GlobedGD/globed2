#pragma once

#include <defs/geode.hpp>

class KeybindSettingsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 200.f;
    static constexpr float CELL_HEIGHT = 30.f;

    static KeybindSettingsPopup* create();

private:
    bool setup();

};
