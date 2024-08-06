#include "list_cell.hpp"
#include <util/gd.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool CollapsableLevelCell::init(GJGameLevel* level, float width) {
    m_level = level;
    this->setContentSize({ width, HEIGHT });

    // initialize levelCell

    m_levelCell = new LevelCell(level->m_levelName.c_str(), width, HEIGHT);
    m_levelCell->autorelease();
    m_levelCell->loadFromLevel(level);
    m_levelCell->setContentSize({ width, HEIGHT });
    m_levelCell->setPosition({ 0.f, 0.f });
    m_levelCell->setVisible(false);

    // initialize collapsedCell

    m_collapsedCell = CCNode::create();

    // (stolen from PlayerListCell)
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

    Build<GJDifficultySprite>::create(util::gd::calcLevelDifficulty(level), GJDifficultyName::Short)
        .scale(0.5f)
        .anchorPoint(0.f, 0.5f)
        .id("difficulty-sprite"_spr)
        .parent(leftMenu);
    
    Build<CCLabelBMFont>::create(level->m_levelName.c_str(), "goldFont.fnt")
        .limitLabelWidth(170.f, 0.6f, 0.1f)
        .intoMenuItem(this, menu_selector(CollapsableLevelCell::onOpenLevel))
        .anchorPoint(0.f, 0.5f)
        .scaleMult(1.1f)
        .parent(leftMenu);

    leftMenu->updateLayout();

    m_collapsedCell->setContentSize({ width, COLLAPSED_HEIGHT });
    m_collapsedCell->setPosition({ 0.f, 0.f });
    m_collapsedCell->setVisible(false);

    this->addChild(m_levelCell);
    this->addChild(m_collapsedCell);

    return true;
}

void CollapsableLevelCell::setIsCollapsed(bool isCollapsed) {
    m_collapsedCell->setVisible(false);
    m_levelCell->setVisible(false);
    if (isCollapsed) {
        m_collapsedCell->setVisible(true);
        this->setContentSize(
            m_collapsedCell->getContentSize()
        );
    } else {
        m_levelCell->setVisible(true);
        this->setContentSize(
            m_levelCell->getContentSize()
        );
    }

    if (auto parent = this->getParent()) {
        parent->setContentSize(this->getContentSize());
    }

    m_isCollapsed = isCollapsed;
}

void CollapsableLevelCell::onOpenLevel(CCObject* sender) {
    util::ui::switchToScene(LevelInfoLayer::create(m_level, false));
}

CollapsableLevelCell* CollapsableLevelCell::create(GJGameLevel* level, float width) {
    auto ret = new CollapsableLevelCell;
    if (ret->init(level, width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ListCellWrapper::init(const PlayerRoomPreviewAccountData& data, float cellWidth, bool forInviting, bool isIconLazyLoad) {
    player_cell = PlayerListCell::create(
        data,
        cellWidth,
        forInviting,
        isIconLazyLoad
    );

    this->setContentSize(player_cell->getContentSize());
    this->addChild(player_cell);

    return true;
}

bool ListCellWrapper::init(GJGameLevel* level, float width, CollapsedCallback callback) {
    room_cell = CollapsableLevelCell::create(level, width);
    this->addChild(room_cell);

    auto rightMenu = Build<CCMenu>::create()
        .pos(width - 20.f, CollapsableLevelCell::COLLAPSED_HEIGHT / 2.f)
        .anchorPoint(0.f, 0.5f)
        .parent(room_cell)
        .id("right-menu")
        .collect();

    float scaleFactor = 0.4f;

    auto collapsedSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
    collapsedSprite->setRotation(-90);
    collapsedSprite->setScale(scaleFactor);
    collapsedSprite->setFlipY(true);

    auto expandedSprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
    expandedSprite->setRotation(90);
    expandedSprite->setScale(scaleFactor);

    Build<CCMenuItemToggler>::createToggle(collapsedSprite, expandedSprite, [this, callback](CCMenuItemToggler* toggler) {
        bool isCollapsed;
        if (!toggler->isOn()) {
            isCollapsed = true;
        } else {
            isCollapsed = false;
        }
        room_cell->setIsCollapsed(isCollapsed);

        if (auto parent = this->getParent()) {
            parent->setContentSize(this->getContentSize());
        }

        callback(room_cell->m_isCollapsed);
    })
        .parent(rightMenu);

    room_cell->setIsCollapsed(false);
    
    return true;
}

ListCellWrapper* ListCellWrapper::create(const PlayerRoomPreviewAccountData& data, float cellWidth, bool forInviting, bool isIconLazyLoad) {
    auto ret = new ListCellWrapper;
    if (ret->init(
        data,
        cellWidth,
        forInviting,
        isIconLazyLoad
    )) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

ListCellWrapper* ListCellWrapper::create(GJGameLevel* level, float width, CollapsedCallback callback) {
    auto ret = new ListCellWrapper;
    if (ret->init(
        level,
        width,
        callback
    )) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

