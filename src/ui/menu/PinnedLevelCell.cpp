#include "PinnedLevelCell.hpp"
#include <globed/util/gd.hpp>

#include <cue/Util.hpp>
#include <UIBuilder.hpp>
#include <asp/iter.hpp>

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

void PinnedLevelCell::hide() {
    this->setContentHeight(0);
    this->invokeCallback();
}

void PinnedLevelCell::loadLevel(int id) {
    if (m_lastLoaded == id) return;
    m_lastLoaded = id;

    // don't load main/tower levels
    if (globed::classifyLevel(id).kind != GameLevelKind::Custom) {
        this->hide();
        return;
    }

    m_level = nullptr;
    cue::resetNode(m_levelCell);

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
    m_circle->setVisible(false);

    if (!level) {
        globed::toastError("(Globed) Failed to load pinned level");
        this->hide();
        return;
    }

    bool useCompact = true;

    m_level = level;

    m_levelCell = new LevelCell(level->m_levelName.c_str(), this->getContentWidth(), HEIGHT);
    m_levelCell->autorelease();
    m_levelCell->m_compactView = useCompact;
    m_levelCell->loadFromLevel(level);
    m_levelCell->setContentSize({ this->getContentWidth(), HEIGHT });
    m_levelCell->setPosition({ 0.f, 0.f });

    if (auto cvoltonID = m_levelCell->m_mainLayer->getChildByIDRecursive("cvolton.betterinfo/level-id-label")) {
        cvoltonID->setVisible(false);
    }

    bool hasCompactListsMod = false;
    if (auto mod = Loader::get()->getLoadedMod("cvolton.compact_lists")) {
        // the mod seems to apply to levelcells unconditionally, doesnt check a setting
        hasCompactListsMod = true;
    } else {
        auto lplace = m_levelCell->m_mainLayer->getChildByID("level-place");
        if (lplace) {
            auto xpos = lplace->getPositionX();
            hasCompactListsMod = xpos < 0.f;
        }
    }

    // we are about to do .. mischievous things
    if (useCompact && !hasCompactListsMod) {
        float shift = 20.f;

        std::array toHide = {
            std::string_view{"level-place"}
        };

        std::array dontShift = {
            std::string_view{"main-menu"}
        };

        for (auto child : m_levelCell->m_mainLayer->getChildrenExt()) {
            bool hide = asp::iter::contains(toHide, child->getID());
            bool doShift = !asp::iter::contains(dontShift, child->getID());

            if (hide) {
                child->setVisible(false);
            }

            if (doShift) {
                child->setPositionX(child->getPositionX() - shift);
            }
        }

        // sobbing bruh
        auto creator = m_levelCell->m_mainLayer->getChildByIDRecursive("creator-name");
        if (creator) {
            creator->setPositionX(creator->getPositionX() - shift);
        }

        CCNode* cper = m_levelCell->m_mainLayer->getChildByID("completed-icon");
        if (!cper) {
            cper = m_levelCell->m_mainLayer->getChildByID("percentage-label");
        }

        auto levelName = m_levelCell->m_mainLayer->getChildByID("level-name");

        if (cper && levelName && creator) {
            // downright disgusting
            auto point = levelName->getPosition();
            point.x += levelName->getScaledContentWidth();
            point.x += 5.f;
            point.x += creator->getScaledContentWidth();
            point.x += 4.f;

            cper->setPosition(point);
            cper->setAnchorPoint({0.f, 0.5f});
        }
    }

    this->addChild(m_levelCell);
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