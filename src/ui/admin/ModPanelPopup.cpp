#include "ModPanelPopup.hpp"
#include "ModNoticeSetupPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ModPanelPopup::POPUP_SIZE { 300.f, 160.f };

bool ModPanelPopup::setup() {
    Build<ButtonSprite>::create("Hello", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .intoMenuItem([this] {
            ModNoticeSetupPopup::create()->show();
        })
        .scaleMult(1.1f)
        .pos(this->center())
        .parent(m_buttonMenu);

    return true;
}

}
