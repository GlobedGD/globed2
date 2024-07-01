#include "daily_popup.hpp"
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <hooks/level_select_layer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <managers/daily_manager.hpp>
#include "Geode/binding/CCMenuItemSpriteExtra.hpp"
#include "download_level_popup.hpp"
#include "../level_list/featured_list_layer.hpp"
#include "daily_level_cell.hpp"

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

    auto blCorner = Build<CCSprite>::createSpriteName("dailyLevelCorner_001.png")
        .scale(1.0f)
        .pos({0, 0})
        .zOrder(3)
        .anchorPoint({0, 0})
        .parent(m_mainLayer);
    
    auto brCorner = Build<CCSprite>::createSpriteName("dailyLevelCorner_001.png")
        .scale(1.0f)
        .flipX(true)
        .zOrder(3)
        .pos({m_mainLayer->getScaledContentSize().width, 0})
        .anchorPoint({1, 0})
        .parent(m_mainLayer);

    auto tlCorner = Build<CCSprite>::createSpriteName("dailyLevelCorner_001.png")
        .scale(1.0f)
        .flipY(true)
        .zOrder(3)
        .pos({0, m_mainLayer->getScaledContentSize().height})
        .anchorPoint({0, 1})
        .parent(m_mainLayer);

    auto trCorner = Build<CCSprite>::createSpriteName("dailyLevelCorner_001.png")
        .scale(1.0f)
        .flipY(true)
        .flipX(true)
        .zOrder(3)
        .pos({m_mainLayer->getScaledContentSize().width, m_mainLayer->getScaledContentSize().height})
        .anchorPoint({1, 1})
        .parent(m_mainLayer);

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
        .pos({m_mainLayer->getScaledContentWidth() / 2, m_mainLayer->getScaledContentHeight() - 30.f})
        .parent(m_mainLayer);

    auto menu = CCMenu::create();
    menu->setPosition({m_mainLayer->getScaledContentSize() / 2});
    m_mainLayer->addChild(menu);

    auto viewAllMenu = Build<CCMenu>::create()
    .parent(m_mainLayer)
    .pos({m_mainLayer->getScaledContentWidth() / 2, 40.f});

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

    DailyManager::get().requestDailyItems();
    DailyItem item = DailyManager::get().getRecentDailyItem();

    auto cell = GlobedDailyLevelCell::create(item.levelId, item.edition, item.rateTier);
    cell->setPosition({m_mainLayer->getScaledContentWidth() / 2, m_mainLayer->getScaledContentHeight() / 2 - 10.f});
    m_mainLayer->addChild(cell);
    
    m_mainLayer->setPositionX(winSize.width * -0.5f);

    auto sequence = CCSequence::create(
        CCEaseElasticOut::create(CCMoveTo::create(0.5f, {winSize.width * 0.5f, winSize.height / 2}), 0.85),
        nullptr
    );

    m_mainLayer->runAction(sequence);

    return true;
}

void DailyPopup::openLevel(CCObject*) {

}

void DailyPopup::onClose(CCObject* self) {
    Popup::onClose(self);
    auto* glm = GameLevelManager::sharedState();
    glm->m_levelDownloadDelegate = nullptr;
    glm->m_levelManagerDelegate = nullptr;
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