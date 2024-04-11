#include "advanced_settings_popup.hpp"
#include <managers/account.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdvancedSettingsPopup::setup() {
    auto rlayout = util::ui::getPopupLayout(m_size);
    this->setTitle("Advanced settings");

    Build<ButtonSprite>::create("Reset token", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            GlobedAccountManager::get().clearAuthKey();
            Notification::create("Successfully cleared the authtoken", NotificationIcon::Success)->show();
        })
        .pos(rlayout.center)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

AdvancedSettingsPopup* AdvancedSettingsPopup::create() {
    auto ret = new AdvancedSettingsPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}