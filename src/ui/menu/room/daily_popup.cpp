#include "daily_popup.hpp"
#include <net/manager.hpp>
#include <util/ui.hpp>
#include <hooks/level_select_layer.hpp>
#include <hooks/gjgamelevel.hpp>
#include "download_level_popup.hpp"
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

    auto title = Build<CCSprite>::createSpriteName("title-daily.png"_spr)
        .scale(0.8f)
        .zOrder(3)
        .pos({m_mainLayer->getScaledContentWidth() / 2, m_mainLayer->getScaledContentHeight() - 30.f})
        .parent(m_mainLayer);

    auto menu = CCMenu::create();
    menu->setPosition({m_mainLayer->getScaledContentSize() / 2});
    m_mainLayer->addChild(menu);

    int levelId = 102837084;
    //int levelId = 76880635;

    auto cell = GlobedDailyLevelCell::create(levelId, 1, 2);
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