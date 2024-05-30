#include "level_browser_layer.hpp"

#include <hooks/level_cell.hpp>
#include <hooks/gjgamelevel.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

void HookedLevelBrowserLayer::setupLevelBrowser(cocos2d::CCArray* p0) {
    LevelBrowserLayer::setupLevelBrowser(p0);

    bool inLists = typeinfo_cast<LevelListLayer*>(this) != nullptr;
    if (inLists) return;

    auto& nm = NetworkManager::get();

    if (!p0 || !nm.established()) return;

    std::vector<LevelId> levelIds;
    for (auto* level_ : CCArrayExt<CCObject*>(p0)) {
        if (!typeinfo_cast<GJGameLevel*>(level_)) continue;

        auto* level = static_cast<GJGameLevel*>(level_);
        if (isValidLevelType(level->m_levelType)) {
            levelIds.push_back(HookedGJGameLevel::getLevelIDFrom(level));
        }
    }

    nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
    nm.addListener<LevelPlayerCountPacket>(this, [this](std::shared_ptr<LevelPlayerCountPacket> packet) {
        for (const auto& [levelId, playerCount] : packet->levels) {
            m_fields->levels[levelId] = playerCount;
        }

        this->refreshPagePlayerCounts();
    });

    this->schedule(schedule_selector(HookedLevelBrowserLayer::updatePlayerCounts), 5.0f);
}

void HookedLevelBrowserLayer::refreshPagePlayerCounts() {
    if (!m_list->m_listView) return;

    bool inLists = typeinfo_cast<LevelListLayer*>(this) != nullptr;

    for (auto* cell_ : CCArrayExt<CCNode*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
        if (!typeinfo_cast<LevelCell*>(cell_)) continue;
        auto* cell = static_cast<GlobedLevelCell*>(cell_);

        if (!isValidLevelType(cell->m_level->m_levelType)) continue;

        LevelId levelId = HookedGJGameLevel::getLevelIDFrom(cell->m_level);
        if (m_fields->levels.contains(levelId)) {
            cell->updatePlayerCount(m_fields->levels.at(levelId), inLists);
        } else {
            cell->updatePlayerCount(-1, inLists);
        }
    }
}

void HookedLevelBrowserLayer::updatePlayerCounts(float) {
    auto& nm = NetworkManager::get();
    if (nm.established() && m_list->m_listView) {
        std::vector<LevelId> levelIds;
        for (auto* cell_ : CCArrayExt<CCNode*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
            if (auto* cell = typeinfo_cast<LevelCell*>(cell_)) {
                if (isValidLevelType(cell->m_level->m_levelType)) {
                    levelIds.push_back(HookedGJGameLevel::getLevelIDFrom(cell->m_level));
                }
            }
        }

        nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
    }
}