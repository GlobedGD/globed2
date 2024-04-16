#include "advanced_settings_popup.hpp"
#include <managers/account.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdvancedSettingsPopup::setup() {
    auto rlayout = util::ui::getPopupLayout(m_size);
    this->setTitle("Advanced settings");

    auto* menu = Build<ButtonSprite>::create("Reset token", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            GlobedAccountManager::get().clearAuthKey();
            Notification::create("Successfully cleared the authtoken", NotificationIcon::Success)->show();
        })
        .pos(rlayout.center)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create("Reset flags", "bigFont.fnt", "GJ_button_01.png", 0.75f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            auto& settings = GlobedSettings::get();
            settings.flags = GlobedSettings::Flags{};
            settings.save();
            Notification::create("Successfully reset all flags", NotificationIcon::Success)->show();
        })
        .pos(rlayout.center - CCPoint{0.f, 30.f})
        .parent(menu);

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