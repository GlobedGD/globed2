#pragma once
#include <defs/geode.hpp>

class AdvancedSettingsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 240.f;
    static constexpr float POPUP_HEIGHT = 120.f;

    static AdvancedSettingsPopup* create();

private:
    bool setup() override;
};
