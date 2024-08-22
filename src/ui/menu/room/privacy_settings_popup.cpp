#include "privacy_settings_popup.hpp"

#include <net/manager.hpp>
#include <managers/settings.hpp>

bool PrivacySettingsPopup::setup() {

    return true;
}

void PrivacySettingsPopup::sendPacket() {
    NetworkManager::get().sendUpdatePlayerStatus(GlobedSettings::get().getPrivacyFlags());
}

PrivacySettingsPopup* PrivacySettingsPopup::create() {
    auto ret = new PrivacySettingsPopup;
    if (ret->initAnchored(240.f, 120.f)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}