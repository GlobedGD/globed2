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

bool UserPunishmentPopup::setup(SimpleTextArea* textArea, int64_t expiresAt, bool isBan) {
    this->setTitle((isBan) ? "You have been banned!" : "You have been muted!");

    Duration punishmentTime = SystemTime::fromUnix(expiresAt).until();

    std::string timeUntil = (punishmentTime.isZero()) ? "Never" : fmt::format("in {}", punishmentTime.toHumanString(1));

    auto sizes = util::ui::getPopupLayoutAnchored(m_size);

    auto layout = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setGap(10.f))
        .contentSize(m_size.width * 0.9f, m_size.height * 0.58f)
        .anchorPoint(0.5f, 0.5f)
        .pos(sizes.fromCenter(0.f, 7.5f))
        .parent(m_mainLayer)
        .collect();

    layout->addChild(textArea);

    Build<CCLabelBMFont>::create(fmt::format("Expires: {}", timeUntil).c_str(), "goldFont.fnt")
        .scale(0.65f)
        .parent(layout);

    layout->updateLayout();

    Build<ButtonSprite>::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.75f)
        .intoMenuItem([this] {
            this->onClose(nullptr);
        })
        .pos(sizes.fromBottom(24.f))
        .parent(m_buttonMenu);

    if (CentralServerManager::get().isOfficialServerActive()) {
        Build<CCSprite>::createSpriteName("gj_discordIcon_001.png")
            .scale(0.85f)
            .intoMenuItem([this] {
                web::openLinkInBrowser(globed::string<"discord">());
            })
            .pos(sizes.fromBottomRight(20.f, 20.f))
            .parent(m_buttonMenu);

        Build<CCLabelBMFont>::create("Appeal:", "bigFont.fnt")
            .scale(0.35f)
            .anchorPoint(1.f, 0.5f)
            .pos(sizes.fromBottomRight(36.f, 16.f))
            .parent(m_buttonMenu);
    }

    return true;
}

bool UserPunishmentPopup::initCustomSize(std::string_view reason, int64_t expiresAt, bool isBan) {
    std::string reasonText = fmt::format("Reason: {}",
        (reason.empty()) ? "No reason provided" : reason);

    float width = 340.f;

    float fontScale = 0.5f;
    // if (reasonText.size() > 64) {
    //     // idk lol
    //     fontScale = 0.5f - 0.001f * (reasonText.size() - 64);
    // }

    auto reasonTextNode = Build<SimpleTextArea>::create(reasonText, "bigFont.fnt", fontScale, width * 0.9f)
        .anchorPoint(0.5f, 0.5f)
        .collect();
    reasonTextNode->setLinePadding(-1.f);
    reasonTextNode->setWrappingMode(WrappingMode::SPACE_WRAP);
    reasonTextNode->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);

    float textAreaHeight = reasonTextNode->getScaledContentHeight();

    return this->initAnchored(width, 120.f + textAreaHeight, reasonTextNode, expiresAt, isBan);
}

void UserPunishmentPopup::onClose(cocos2d::CCObject* a) {
    Popup::onClose(a);

    SceneManager::get()->forget(this);
}

UserPunishmentPopup* UserPunishmentPopup::create(std::string_view reason, int64_t expiresAt, bool isBan) {
    auto ret = new UserPunishmentPopup();
    if (ret->initCustomSize(reason, expiresAt, isBan)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}