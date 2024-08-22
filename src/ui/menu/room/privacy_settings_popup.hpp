#pragma once

#include <defs/geode.hpp>

class PrivacySettingsPopup : public geode::Popup<> {
public:
    static PrivacySettingsPopup* create();

protected:
    bool setup() override;

    void sendPacket();
};
