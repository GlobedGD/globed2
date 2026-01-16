#include "ModPanelPopup.hpp"
#include "ModAuditLogPopup.hpp"
#include "ModNoticeSetupPopup.hpp"
#include "ModUserPopup.hpp"

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ModPanelPopup::POPUP_SIZE{300.f, 128.f};

bool ModPanelPopup::setup()
{
    this->setTitle("Admin Panel");
    m_title->setPositionY(m_title->getPositionY() + 3.f);

    auto playerContainer = Build<CCMenu>::create()
                               .layout(RowLayout::create()->setAutoScale(false))
                               .anchorPoint(0.5f, 0.5f)
                               .contentSize(270.f, 30.f)
                               .pos(this->fromCenter(0.f, 12.f))
                               .parent(m_mainLayer)
                               .collect();

    m_queryInput = Build<TextInput>::create(200.f, "Username / ID", "chatFont.fnt").parent(playerContainer);

    Build<CCSprite>::createSpriteName("GJ_longBtn05_001.png")
        .scale(0.9f)
        .intoMenuItem([this] {
            auto query = geode::utils::string::trim(m_queryInput->getString());

            if (query.empty()) {
                return;
            }

            auto popup = ModUserPopup::create(0);
            popup->startLoadingProfile(query, false);
            popup->show();
        })
        .scaleMult(1.15f)
        .parent(playerContainer);

    playerContainer->updateLayout();

    auto btnContainer = Build<CCMenu>::create()
                            .layout(RowLayout::create())
                            .anchorPoint(0.5f, 0.5f)
                            .contentSize(270.f, 35.f)
                            .pos(this->fromBottom(30.f))
                            .parent(m_mainLayer)
                            .collect();

    Build<ButtonSprite>::create("Notice", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem(+[] { ModNoticeSetupPopup::create()->show(); })
        .scaleMult(1.1f)
        .parent(btnContainer);

    Build<ButtonSprite>::create("Logs", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.9f)
        .intoMenuItem(+[] { ModAuditLogPopup::create()->show(); })
        .scaleMult(1.1f)
        .parent(btnContainer);

    btnContainer->updateLayout();

    return true;
}

} // namespace globed
