#include "user_punishment_popup.hpp"
#include <Geode/Geode.hpp>

#include <asp/time.hpp>
#include <fmt/format.h>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using namespace asp::time;

bool UserPunishmentPopup::setup(UserPunishment const& punishment) {
    bool isBan = (punishment.type == PunishmentType::Ban) ? true : false;

    this->setTitle((isBan) ? "You have been banned!" : "You have been muted!");

    CCMenu* punishedMenu;

    // Build reason/expiry/discord text
    Build<CCMenu>::create()
        .id("punishment-menu")
        .contentSize(350.f, 120.f)
        .anchorPoint(ccp(0.5f, 0.5f))
        .parent(m_mainLayer)
        .pos(25.f, 55.f)
        .store(punishedMenu)
        .intoNewChild(CCLabelBMFont::create(fmt::format("Reason: {}", 
            (punishment.reason.empty()) ? "No reason provided" : punishment.reason).c_str(), "bigFont.fnt"))
            .id("reason-text")
            .anchorPoint(0.f, 0.5f)
            .scale(0.5f)
            .posY(120.f)
        .intoNewSibling(CCLabelBMFont::create(fmt::format("Expires at: {}",
            util::format::dateTime(SystemTime::fromUnix(punishment.expiresAt))).c_str(), "bigFont.fnt"))
            .id("expires-at-text")
            .anchorPoint(0.f, 0.5f)
            .scale(0.5f)
            .posY(90.f)
        .intoNewSibling(CCLabelBMFont::create("Questions/Appeals? Join the Discord", "bigFont.fnt"))
            .id("questions-appeals-text")
            .anchorPoint(0.f, 0.5f)
            .scale(0.5f)
            .posY(60.f);

        auto rlayout = util::ui::getPopupLayoutAnchored(m_size);
        auto* rootLayout = Build<CCNode>::create()
            .contentSize(m_size.width * 0.9f, m_size.height * 0.7f)
            .pos(rlayout.fromCenter(0.f, -20.f))
            .anchorPoint(0.5f, 0.5f)
            .layout(
                ColumnLayout::create()
                    ->setAutoScale(false)
                    ->setAxisReverse(true)
                )
            .parent(m_mainLayer)
            .collect();
        auto* menu = Build<CCMenu>::create()
            .layout(RowLayout::create()->setAutoScale(true))
            .parent(rootLayout)
            .collect();

        
    return true;
}

UserPunishmentPopup* UserPunishmentPopup::create(UserPunishment const& punishment) {
    auto ret = new UserPunishmentPopup();
    if (ret->initAnchored(400.f, 240.f, punishment)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}