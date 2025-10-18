#include "FeaturedPopup.hpp"
#include <globed/util/gd.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <ui/menu/FeaturedListLayer.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize FeaturedPopup::POPUP_SIZE {420.f, 280.f};

bool FeaturedPopup::setup() {
    this->setID("daily-popup"_spr);

    auto& nm = NetworkManagerImpl::get();
    if (!nm.isConnected()) {
        return false;
    }

    m_noElasticity = true;

    geode::addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold);

    CCMenu* blCornerMenu = Build<CCMenu>::create()
        .pos(20.f, 20.f)
        .zOrder(5)
        .parent(m_mainLayer);

    auto infoBtn = Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png")
        .intoMenuItem(+[] {
            globed::alert(
                "Featured Guide",

                "Globed will occasionally <co>highlight Platformer levels</c> made by the community.\n\n"
                "There are three feature types:\n<cl>Normal</c>, <cj>Epic</c>, and <cg>Outstanding</c>."
            );
        })
        .parent(blCornerMenu);

    auto title = Build<CCSprite>::create("title-daily.png"_spr)
        .scale(1.0f)
        .zOrder(3)
        .pos(this->fromTop(30.f))
        .parent(m_mainLayer);

    auto menu = CCMenu::create();
    menu->setPosition(this->center());
    m_mainLayer->addChild(menu);

    auto viewAllMenu = Build<CCMenu>::create()
        .parent(m_mainLayer)
        .pos(this->fromBottom(40.f));

    auto viewAllBtnSpr = Build<CCSprite>::createSpriteName("GJ_longBtn03_001.png")
        .intoMenuItem([this](auto) {
            auto layer = FeaturedListLayer::create();
            globed::pushScene(layer);
        })
        .parent(viewAllMenu)
        .collect();

    auto viewAllBtnStar = Build<CCSprite>::createSpriteName("GJ_bigStar_001.png")
        .parent(viewAllBtnSpr)
        .pos(20.f, viewAllBtnSpr->getScaledContentHeight() / 2 + 1.25f)
        .scale(0.4);

    auto viewAllLabel = Build<CCLabelBMFont>::create("Featured List", "bigFont.fnt")
        .parent(viewAllBtnSpr)
        .pos({36.f, viewAllBtnSpr->getScaledContentHeight() / 2 + 2.f})
        .anchorPoint({0, 0.5})
        .scale(0.5);

    auto winSize = CCDirector::get()->getWinSize();

    m_cell = FeaturedLevelCell::create();
    m_cell->setPosition(this->fromCenter(0.f, -10.f));
    m_mainLayer->addChild(m_cell);
    m_mainLayer->setPositionX(winSize.width * -0.5f);

    auto sequence = CCSequence::create(
        CCEaseElasticOut::create(CCMoveTo::create(0.5f, {winSize.width * 0.5f, winSize.height / 2}), 0.85),
        nullptr
    );

    m_mainLayer->runAction(sequence);

    // refresh button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this] {
            auto& nm = NetworkManagerImpl::get();
            m_cell->reloadFull();
        })
        .id("refresh-btn")
        .pos(this->fromBottomRight(-2.f, 2.f))
        .intoNewParent(CCMenu::create())
        .id("refresh-menu")
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

}
