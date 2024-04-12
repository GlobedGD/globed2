#include "credits_cell.hpp"
#include "credits_popup.hpp"
#include "credits_player.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedCreditsCell::init(const char* name, bool lightBg, cocos2d::CCArray* players) {
    if (!CCNode::init()) return false;

    constexpr float cellWidth = GlobedCreditsPopup::LIST_WIDTH;

    auto* title = Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .scale(0.7f)
        .pos(cellWidth / 2, 0.f)
        .parent(this)
        .collect();

    float wrapperWidth = cellWidth;
    if (players->count() > 6) {
        wrapperWidth = cellWidth / 1.15f;
    }

    float playerGap = 15.f;
    if (players->count() < 4) {
        playerGap = 40.f;
    } else if (players->count() == 4) {
        playerGap = 30.f;
    } else if (players->count() == 5) {
        playerGap = 25.f;
    } else if (players->count() == 6) {
        playerGap = 18.f;
    } else {
        wrapperWidth = cellWidth / 1.15f;
        playerGap = 22.f;
    }

    auto* playerWrapper = Build<CCNode>::create()
        .layout(RowLayout::create()->setGap(playerGap)->setGrowCrossAxis(true))
        .pos(cellWidth / 2.f, 0.f)
        .anchorPoint(0.5f, 0.0f)
        .contentSize(wrapperWidth, 0.f)
        .parent(this)
        .collect();

    for (auto* player : CCArrayExt<GlobedCreditsPlayer*>(players)) {
        playerWrapper->addChild(player);
    }

    playerWrapper->updateLayout();

    this->setContentSize(CCSize{cellWidth, playerWrapper->getScaledContentSize().height + 5.f + title->getScaledContentSize().height});

    Build<CCLayerColor>::create(lightBg ? util::ui::BG_COLOR_BROWN : util::ui::BG_COLOR_DARKBROWN)
        .id("background")
        .zOrder(-1)
        .parent(this)
        .contentSize(this->getContentSize());

    // title at the top
    title->setPosition({title->getPositionX(), this->getContentHeight() - 10.f});

    return true;
}

GlobedCreditsCell* GlobedCreditsCell::create(const char* name, bool lightBg, cocos2d::CCArray* players) {
    auto ret = new GlobedCreditsCell;
    if (ret->init(name, lightBg, players)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}