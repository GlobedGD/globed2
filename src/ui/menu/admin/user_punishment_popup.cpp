#include "user_punishment_popup.hpp"
#include <defs/util.hpp>
#include <managers/central_server.hpp>
#include <data/types/user.hpp>
#include <hooks/gjbasegamelayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <asp/time.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;
using namespace asp::time;

bool UserPunishmentPopup::setup(std::string reason, int64_t expiresAt, bool isBan) {
    this->setTitle((isBan) ? "You have been banned!" : "You have been muted!");

    CCMenu* punishedMenu;

    Duration punishmentTime = SystemTime::fromUnix(expiresAt)
        .durationSince(SystemTime::now())
        .value_or(Duration{});

    std::string timeUntil = (punishmentTime.isZero()) ? "Never" : fmt::format("in {}", punishmentTime.toHumanString(1));
    
    std::string reasonText = fmt::format("Reason: {}", 
        (reason.empty()) ? "No reason provided" : reason);

    auto sizes = util::ui::getPopupLayoutAnchored(m_size);
    
    auto layout = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setGap(10.f))
        .contentSize(m_size.width * 0.9f, m_size.height * 0.6f)
        .anchorPoint(0.5f, 0.5f)
        .pos(sizes.fromCenter(0.f, 10.f))
        .parent(m_mainLayer);

    auto reasonTextNode = Build<SimpleTextArea>::create(reasonText, "bigFont.fnt", 0.5f, m_size.width * 0.9f)
        .anchorPoint(0.5f, 0.5f)
        .parent(layout)
        .collect();
    reasonTextNode->setWrappingMode(WrappingMode::SPACE_WRAP);
    reasonTextNode->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);

    Build<CCLabelBMFont>::create(fmt::format("Expires: {}", timeUntil).c_str(), "goldFont.fnt")
        .scale(0.65f)
        .parent(layout);
    
    layout.updateLayout();

    auto* menu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(true))
        .collect();

    Build<ButtonSprite>::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.8f)
        .intoMenuItem([this] {
            this->onClose(nullptr);
        })
        .parent(menu);
    
    auto discordButton = Build<CCSprite>::createSpriteName("gj_discordIcon_001.png")
        .intoMenuItem([this] {
            web::openLinkInBrowser(globed::string<"discord">());
        })
        .intoNewParent(CCMenu::create())
        .collect();

    menu->updateLayout();
    m_mainLayer->addChildAtPosition(menu, Anchor::Bottom, ccp(0.f, 25.f));

    if (CentralServerManager::get().isOfficialServerActive()) {
        m_mainLayer->addChildAtPosition(discordButton, Anchor::BottomRight, ccp(-25.f, 25.f));
    }
        
    return true;
}

UserPunishmentPopup* UserPunishmentPopup::create(std::string reason, int64_t expiresAt, bool isBan) {
    auto ret = new UserPunishmentPopup();
    if (ret->initAnchored(300.f, 160.f, reason, expiresAt, isBan)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}