#include "credits_cell.hpp"
#include "credits_popup.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedCreditsCell::init(const char* name, bool lightBg, cocos2d::CCArray* players) {
    if (!CCNode::init()) return false;

    constexpr float cellWidth = GlobedCreditsPopup::LIST_WIDTH;
    constexpr size_t maxPlayersInRow = 6;

    size_t rows = (players->count() + maxPlayersInRow - 1) / maxPlayersInRow;

    auto* title = Build<CCLabelBMFont>::create(name, "bigFont.fnt")
        .scale(0.68f)
        .pos(cellWidth / 2, 3.f)
        .parent(this)
        .collect();

    const float wrapperGap = 5.f;
    auto* playerWrapper = Build<CCNode>::create()
        .layout(ColumnLayout::create()->setAxisReverse(true)->setGap(wrapperGap))
        .pos(cellWidth / 2.f, 0.f)
        .anchorPoint(0.5f, 0.0f)
        .contentSize(cellWidth, 50.f)
        .id("player-wrapper"_spr)
        .parent(this)
        .collect();

    float wrapperHeight = 0.f;

    for (size_t i = 0; i < rows; i++) {
        size_t firstIdx = i * maxPlayersInRow;
        size_t lastIdx = std::min((size_t)((i + 1) * maxPlayersInRow), (size_t)players->count());
        size_t count = lastIdx - firstIdx;

        float playerGap = 15.f;
        if (count < 4) {
            playerGap = 40.f;
        } else if (count == 4) {
            playerGap = 30.f;
        } else if (count == 5) {
            playerGap = 25.f;
        } else if (count == 6) {
            playerGap = 18.f;
        }

        auto* row = Build<CCNode>::create()
            .layout(RowLayout::create()->setGap(playerGap))
            .id("wrapper-row"_spr)
            .parent(playerWrapper)
            .contentSize(cellWidth, 0.f)
            .collect();

        for (size_t i = firstIdx; i < lastIdx; i++) {
            row->addChild(static_cast<CCNode*>(players->objectAtIndex(i)));
        }

        row->updateLayout();

        wrapperHeight += row->getContentHeight();
        if (i != 0) {
            wrapperHeight += wrapperGap;
        }
    }

    playerWrapper->setContentSize({0.f, wrapperHeight});
    playerWrapper->updateLayout();

    this->setContentSize(CCSize{cellWidth, playerWrapper->getScaledContentSize().height + 8.f + title->getScaledContentSize().height});

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