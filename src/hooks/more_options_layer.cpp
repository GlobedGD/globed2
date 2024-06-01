#include <defs/platform.hpp>

#ifdef GEODE_IS_ANDROID

#include "more_options_layer.hpp"

#include <ui/menu/admin/admin_login_popup.hpp>
#include <ui/menu/admin/admin_popup.hpp>
#include <managers/admin.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

bool HookedMoreOptionsLayer::init() {
    if (!MoreOptionsLayer::init()) return false;

    m_fields->adminBtn = Build<ButtonSprite>::create("admin", "goldFont.fnt", "GJ_button_04.png", 0.75f)
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            if (AdminManager::get().authorized()) {
                AdminPopup::create()->show();
            } else {
                AdminLoginPopup::create()->show();
            }
        })
        .pos(-35.f, -110.f)
        .parent(m_buttonMenu)
        .visible(false)
        .collect();

    return true;
}

void HookedMoreOptionsLayer::goToPage(int page) {
    MoreOptionsLayer::goToPage(page);

    auto* bumpscosity = Loader::get()->getLoadedMod("colon.bumpscosity");
    if (!bumpscosity) return;

    float value = bumpscosity->getSavedValue<float>("bumpscosity");

    if (m_fields->adminBtn) {
        bool isCorrect = value >= 0.45f && value < 0.47f;
        m_fields->adminBtn->setVisible(isCorrect && page == 4);
    }
}

#endif // GEODE_IS_ANDROID