#include "more_options_layer.hpp"

#ifdef GEODE_IS_ANDROID

#include <ui/menu/admin/admin_login_popup.hpp>
#include <ui/menu/admin/admin_popup.hpp>
#include <net/network_manager.hpp>

using namespace geode::prelude;

bool HookedMoreOptionsLayer::init() {
    if (!MoreOptionsLayer::init()) return false;

    Build<ButtonSprite>::create("admin", "goldFont.fnt", "GJ_button_04.png", 0.75f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            if (NetworkManager::get().isAuthorizedAdmin()) {
                AdminPopup::create()->show();
            } else {
                AdminLoginPopup::create()->show();
            }
        })
        .pos(-35.f, -110.f)
        .parent(m_buttonMenu)
        .visible(false)
        .store(m_fields->adminBtn);

    return true;
}

void HookedMoreOptionsLayer::goToPage(int x) {
    log::debug("hii button: {}", m_fields->adminBtn);
    if (m_fields->adminBtn) {
        m_fields->adminBtn->setVisible(x == 4);
    }

    MoreOptionsLayer::goToPage(x);
}

#endif // GEODE_IS_ANDROID