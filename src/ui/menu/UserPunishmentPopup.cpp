#include "UserPunishmentPopup.hpp"

#include <UIBuilder.hpp>
#include <asp/time.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

bool UserPunishmentPopup::initCustomSize(const std::string& reason, int64_t expiresAt, bool isBan) {
    constexpr float WIDTH = 340.f;
    auto reasonText = fmt::format("Reason: {}", reason.empty() ? "No reason provided" : reason);

    m_textArea = Build<SimpleTextArea>::create(reasonText, "bigFont.fnt", 0.5f, WIDTH * 0.9f)
        .anchorPoint(0.5f, 0.5f);
    m_textArea->setLinePadding(-1.f);
    m_textArea->setWrappingMode(WrappingMode::SPACE_WRAP);
    m_textArea->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);

    m_expiresAt = expiresAt;
    m_isBan = isBan;

    float taHeight = m_textArea->getScaledContentHeight();

    return this->initAnchored(WIDTH, 120.f + taHeight);
}

bool UserPunishmentPopup::setup() {
    this->setTitle(m_isBan ? "You have been banned!" : "You have been muted!");

    Duration punishmentTime = SystemTime::fromUnix(m_expiresAt).until();

    std::string timeUntil = punishmentTime.isZero() ? "Never" : fmt::format("in {}", punishmentTime.toHumanString(1));

    auto layout = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setAxisReverse(true)->setAutoScale(false)->setGap(10.f))
        .contentSize(m_size.width * 0.9f, m_size.height * 0.58f)
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromCenter(0.f, 7.5f))
        .parent(m_mainLayer)
        .collect();

    layout->addChild(m_textArea);

    Build<CCLabelBMFont>::create(fmt::format("Expires: {}", timeUntil).c_str(), "goldFont.fnt")
        .scale(0.65f)
        .parent(layout);

    layout->updateLayout();

    Build<ButtonSprite>::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .scale(0.75f)
        .intoMenuItem([this] {
            this->onClose(nullptr);
        })
        .pos(this->fromBottom(24.f))
        .parent(m_buttonMenu);

    // TODO: dont show this on a custom server
    if (true) {
        Build<CCSprite>::createSpriteName("gj_discordIcon_001.png")
            .scale(0.85f)
            .intoMenuItem(+[] {
                web::openLinkInBrowser(globed::constant<"discord">());
            })
            .pos(this->fromBottomRight(20.f, 20.f))
            .parent(m_buttonMenu);

        Build<CCLabelBMFont>::create("Appeal:", "bigFont.fnt")
            .scale(0.35f)
            .anchorPoint(1.f, 0.5f)
            .pos(this->fromBottomRight(36.f, 16.f))
            .parent(m_buttonMenu);
    }

    return true;
}

UserPunishmentPopup* UserPunishmentPopup::create(const std::string& reason, int64_t expiresAt, bool isBan) {
    auto ret = new UserPunishmentPopup;
    if (ret->initCustomSize(reason, expiresAt, isBan)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}