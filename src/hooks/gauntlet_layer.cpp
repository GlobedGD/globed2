#include "gauntlet_layer.hpp"

#ifndef GLOBED_LESS_BINDINGS

#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

bool HookedGauntletLayer::init(GauntletType type) {
    if (!GauntletLayer::init(type)) return false;

    if (this->m_levels != nullptr) {
        this->buildUI();
    }

    return true;
}

void HookedGauntletLayer::loadLevelsFinished(CCArray* p0, char const* p1, int p2) {
    GauntletLayer::loadLevelsFinished(p0, p1, p2);
    this->buildUI();
}

void HookedGauntletLayer::buildUI() {
    auto& nm = NetworkManager::get();
    if (!nm.established()) return;

    std::vector<LevelId> levelIds;
    for (auto level : CCArrayExt<GJGameLevel*>(this->m_levels)) {
        levelIds.push_back(level->m_levelID);
    }

    auto levelsMenu = this->getChildByIDRecursive("levels-menu");
    if (levelsMenu == nullptr) {
        return;
    }

    auto levelButtons = CCArrayExt<CCMenuItemSpriteExtra*>(levelsMenu->getChildren());
    for (size_t i = 0; i < levelButtons.size(); i++) {
        auto wrapper = Build<CCNode>::create()
            .pos(15.f, -33.f)
            .scale(0.45f)
            .visible(false)
            .contentSize(levelButtons[i]->getScaledContentSize())
            .layout(RowLayout::create()->setGap(5.f))
            .parent(levelButtons[i])
            .id("level-playercount-wrapper"_spr)
            .collect();

        Build<CCSprite>::createSpriteName("icon-person.png"_spr)
            .id("level-playercount-icon"_spr)
            .parent(wrapper);

        Build<CCLabelBMFont>::create("", "goldFont.fnt")
            .id("level-playercount-label"_spr)
            .parent(wrapper);

        wrapper->updateLayout();
        m_fields->wrappers[levelIds[i]] = wrapper;
    }

    nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
    nm.addListener<LevelPlayerCountPacket>(this, [this](std::shared_ptr<LevelPlayerCountPacket> packet) {
        for (const auto& [levelId, playerCount] : packet->levels) {
            m_fields->levels[levelId] = playerCount;
        }

        this->refreshPlayerCounts();
    });

    this->schedule(schedule_selector(HookedGauntletLayer::updatePlayerCounts), 5.f);
    this->updatePlayerCounts(0.f);
}

void HookedGauntletLayer::refreshPlayerCounts() {
    for (const auto& [levelId, wrapper] : m_fields->wrappers) {
        auto playerCount = m_fields->levels[levelId];

        if (playerCount == 0) {
            wrapper->setVisible(false);
        } else {
            wrapper->setVisible(true);
            auto label = static_cast<CCLabelBMFont*>(wrapper->getChildByID("level-playercount-label"_spr));
            label->setString(fmt::to_string(playerCount).c_str());
            wrapper->updateLayout();
        }
    }
}

void HookedGauntletLayer::updatePlayerCounts(float) {
    auto& nm = NetworkManager::get();
    if (!nm.established()) return;

    std::vector<LevelId> levelIds;

    for (auto level : CCArrayExt<GJGameLevel*>(this->m_levels)) {
        levelIds.push_back(level->m_levelID);
    }

    nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
}

#endif // GLOBED_LESS_BINDINGS
