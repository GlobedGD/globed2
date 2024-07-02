#include "daily_popup.hpp"

#include "daily_level_cell.hpp"
#include "featured_list_layer.hpp"
#include "edit_featured_level_popup.hpp"
#include <managers/daily_manager.hpp>
#include <managers/admin.hpp>
#include <net/manager.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool DailyPopup::setup() {
    this->setID("daily-popup"_spr);

    auto& nm = NetworkManager::get();
    if (!nm.established()) {
        return false;
    }

    m_noElasticity = true;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    // side art
    geode::addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold);

    CCMenu* blCornerMenu = Build<CCMenu>::create()
        .pos(20.f, 20.f)
        .zOrder(5)
        .parent(m_mainLayer);

    auto infoBtn = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .intoMenuItem([](auto) {
            FLAlertLayer::create(
                        "Featured Guide",
                        "Globed will occasionally <co>highlight Platformer levels</c> made by the community.\n\nThe three feature types are:\n<cl>Normal</c>, <cj>Epic</c>, and <cg>Outstanding</c>\n\nSuggest levels on our <cb>Discord server</c> for a chance at being selected!",
                    "Ok")->show();
        })
        .parent(blCornerMenu);

    auto title = Build<CCSprite>::createSpriteName("title-daily.png"_spr)
        .scale(1.0f)
        .zOrder(3)
        .pos(rlayout.fromTop(30.f))
        .parent(m_mainLayer);

    auto menu = CCMenu::create();
    menu->setPosition(rlayout.center);
    m_mainLayer->addChild(menu);

    auto viewAllMenu = Build<CCMenu>::create()
        .parent(m_mainLayer)
        .pos(rlayout.fromBottom(40.f));

    CCMenuItemSpriteExtra* viewAllBtnSpr = Build<CCSprite>::createSpriteName("GJ_longBtn03_001.png")
        .intoMenuItem([this](auto) {
                auto layer = GlobedFeaturedListLayer::create();
                util::ui::switchToScene(layer);
            })
        .parent(viewAllMenu);

    auto viewAllBtnStar = Build<CCSprite>::createSpriteName("GJ_bigStar_001.png")
        .parent(viewAllBtnSpr)
        .pos(20.f, viewAllBtnSpr->getScaledContentHeight() / 2 + 1.25f)
        .scale(0.4);

    auto viewAllLabel = Build<CCLabelBMFont>::create("Featured List", "bigFont.fnt")
        .parent(viewAllBtnSpr)
        .pos({36.f, viewAllBtnSpr->getScaledContentHeight() / 2 + 2.f})
        .anchorPoint({0, 0.5})
        .scale(0.5);

    auto cell = GlobedDailyLevelCell::create();
    cell->setPosition(rlayout.center - CCPoint{0.f, 10.f});
    m_mainLayer->addChild(cell);

    m_mainLayer->setPositionX(winSize.width * -0.5f);

    auto sequence = CCSequence::create(
        CCEaseElasticOut::create(CCMoveTo::create(0.5f, {winSize.width * 0.5f, winSize.height / 2}), 0.85),
        nullptr
    );

    m_mainLayer->runAction(sequence);

    if (AdminManager::get().authorized()) {
        auto& role = AdminManager::get().getRole();
        // TODO: check if edit levels perm is available

        Build<CCSprite>::createSpriteName("accountBtn_settings_001.png")
            .scale(0.7f)
            .intoMenuItem([this] {
                EditFeaturedLevelPopup::create()->show();
            })
            .pos(rlayout.topRight - CCPoint{20.f, 20.f})
            .intoNewParent(CCMenu::create())
            .pos(0.f, 0.f)
            .parent(m_mainLayer);
    }

    return true;
}

void DailyPopup::openLevel(CCObject*) {

}

DailyPopup* DailyPopup::create() {
    auto ret = new DailyPopup();

    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, "GJ_square04.png")) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}