#include "kofi_popup.hpp"

#include <util/ui.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

bool GlobedKofiPopup::setup(CCSprite* bg) {
    m_bgSprite->removeFromParent();

    auto rlayout = util::ui::getPopupLayout(m_size);

    auto winSize = CCDirector::get()->getWinSize();
    bg->setAnchorPoint({0.f, 0.f});
    m_mainLayer->addChild(bg);

    Build<ButtonSprite>::create("Open Ko-fi", "goldFont.fnt", "GJ_button_01.png", 1.f)
        .intoMenuItem([] {
            geode::utils::web::openLinkInBrowser("https://ko-fi.com/globed");
        })
        .pos(m_size.width / 2, 5.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

GlobedKofiPopup* GlobedKofiPopup::create() {
    auto bg = CCSprite::create("kofi-promo-bg.png"_spr);
    bg->setScale(1.35f);
    auto csize = bg->getScaledContentSize();

    auto ret = new GlobedKofiPopup();
    if (ret->initAnchored(csize.width, csize.height, bg)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
