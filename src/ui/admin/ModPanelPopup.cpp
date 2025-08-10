#include "ModPanelPopup.hpp"
#include "ModNoticeSetupPopup.hpp"
#include "ModAuditLogPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ModPanelPopup::POPUP_SIZE { 300.f, 150.f };

bool ModPanelPopup::setup() {
    auto playerContainer = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false))
        .anchorPoint(0.5f, 0.5f)
        .contentSize(270.f, 30.f)
        .pos(this->fromCenter(0.f, 30.f))
        .parent(m_mainLayer)
        .collect();

    Build<TextInput>::create(200.f, "Username / ID", "chatFont.fnt")
        .parent(playerContainer);

    Build<CCSprite>::createSpriteName("GJ_longBtn05_001.png")
        .scale(0.9f)
        .parent(playerContainer);

    playerContainer->updateLayout();

    auto btnContainer = Build<CCMenu>::create()
        .layout(RowLayout::create())
        .anchorPoint(0.5f, 0.5f)
        .contentSize(270.f, 35.f)
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer)
        .collect();

    Build<ButtonSprite>::create("Notice", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem([this] {
            ModNoticeSetupPopup::create()->show();
        })
        .scaleMult(1.1f)
        .parent(btnContainer);

    Build<ButtonSprite>::create("Logs", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem([this] {
            ModAuditLogPopup::create()->show();
        })
        .scaleMult(1.1f)
        .parent(btnContainer);

    btnContainer->updateLayout();

    return true;
}

}
