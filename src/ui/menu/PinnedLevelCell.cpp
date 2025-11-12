#include "PinnedLevelCell.hpp"
#include <globed/util/gd.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool PinnedLevelCell::init(float width) {
    CCNode::init();

    this->setContentSize({ width, HEIGHT });

    // create the loading circle

    m_circle = Build(cue::LoadingCircle::create(false))
        .scale(0.5f);
    m_circle->addToLayer(this);

    return true;
}

void PinnedLevelCell::setCollapsed(bool collapsed) {
    m_collapsed = collapsed;

    if (m_collapsedCell) {
        m_collapsedCell->setVisible(collapsed);
    }

    if (m_levelCell) {
        m_levelCell->setVisible(!collapsed);
    }

    this->setContentHeight(collapsed ? COLLAPSED_HEIGHT : HEIGHT);

    this->invokeCallback();
}

void PinnedLevelCell::loadLevel(int id) {
    if (m_level && m_level->m_levelID == id) return;

    m_level = nullptr;
    cue::resetNode(m_levelCell);
    cue::resetNode(m_collapsedCell);

    this->setContentHeight(HEIGHT);
    m_circle->fadeIn();

    this->invokeCallback();

    globed::getOnlineLevel(id, [ref = WeakRef(this)](GJGameLevel* level) {
        auto self = ref.lock();
        if (!self) return;

        self->onLevelLoaded(level);
    });
}

void PinnedLevelCell::onLevelLoaded(GJGameLevel* level) {
    m_circle->fadeOut();

    if (!level) {
        globed::alert("Error", "Failed to load pinned level.");
        this->setContentHeight(0);
        this->invokeCallback();
        return;
    }

    m_level = level;

    m_levelCell = new LevelCell(level->m_levelName.c_str(), this->getContentWidth(), HEIGHT);
    m_levelCell->autorelease();
    m_levelCell->loadFromLevel(level);
    m_levelCell->setContentSize({ this->getContentWidth(), HEIGHT });
    m_levelCell->setPosition({ 0.f, 0.f });
    m_levelCell->setVisible(false);

    if (auto cvoltonID = m_levelCell->m_mainLayer->getChildByIDRecursive("cvolton.betterinfo/level-id-label")) {
        cvoltonID->setVisible(false);
    }

    // collapsed cel

    m_collapsedCell = CCNode::create();

    auto leftMenu = Build<CCMenu>::create()
        .pos(10.f, COLLAPSED_HEIGHT / 2.f)
        .anchorPoint(0.f, 0.5f)
        .layout(RowLayout::create()
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setGap(10.f)
        )
        .parent(m_collapsedCell)
        .id("left-menu")
        .collect();

    Build<GJDifficultySprite>::create((int) globed::calcLevelDifficulty(level), GJDifficultyName::Short)
        .scale(0.5f)
        .anchorPoint(0.f, 0.5f)
        .id("difficulty-sprite"_spr)
        .parent(leftMenu);

    Build<CCLabelBMFont>::create(level->m_levelName.c_str(), "goldFont.fnt")
        .limitLabelWidth(170.f, 0.6f, 0.1f)
        .intoMenuItem([this] {
            globed::pushScene(LevelInfoLayer::create(m_level, false));
        })
        .anchorPoint(0.5f, 0.5f)
        .scaleMult(1.1f)
        .parent(leftMenu);

    leftMenu->updateLayout();

    m_collapsedCell->setContentSize({ this->getContentWidth(), COLLAPSED_HEIGHT });
    m_collapsedCell->setPosition({ 0.f, 0.f });
    m_collapsedCell->setVisible(false);

    this->addChild(m_levelCell);
    this->addChild(m_collapsedCell);

    this->setCollapsed(m_collapsed);
}

PinnedLevelCell* PinnedLevelCell::create(float width) {
    auto ret = new PinnedLevelCell();
    if (ret->init(width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}