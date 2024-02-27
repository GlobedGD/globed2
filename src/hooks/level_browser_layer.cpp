#include "level_browser_layer.hpp"

#include <hooks/level_cell.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/network_manager.hpp>

using namespace geode::prelude;

void HookedLevelBrowserLayer::setupLevelBrowser(cocos2d::CCArray* p0) {
    LevelBrowserLayer::setupLevelBrowser(p0);
    auto& nm = NetworkManager::get();

    if (!p0 || !nm.established()) return;

    std::vector<LevelId> levelIds;
    for (auto* level_ : CCArrayExt<CCObject*>(p0)) {
        if (!typeinfo_cast<GJGameLevel*>(level_)) continue;

        auto* level = static_cast<GJGameLevel*>(level_);
        if (isValidLevelType(level->m_levelType)) {
            levelIds.push_back(level->m_levelID);
        }
    }

    nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
    nm.addListener<LevelPlayerCountPacket>([this](std::shared_ptr<LevelPlayerCountPacket> packet) {
        for (const auto& [levelId, playerCount] : packet->levels) {
            m_fields->levels[levelId] = playerCount;
        }

        this->refreshPagePlayerCounts();
    });

    this->schedule(schedule_selector(HookedLevelBrowserLayer::updatePlayerCounts), 5.0f);
}

void HookedLevelBrowserLayer::refreshPagePlayerCounts() {
    for (auto* cell_ : CCArrayExt<CCNode*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
        if (!typeinfo_cast<LevelCell*>(cell_)) continue;
        auto* cell = static_cast<GlobedLevelCell*>(cell_);

        if (!isValidLevelType(cell->m_level->m_levelType)) continue;

        if (m_fields->levels.contains(cell->m_level->m_levelID)) {
            cell->updatePlayerCount(m_fields->levels.at(cell->m_level->m_levelID));
        } else {
            cell->updatePlayerCount(-1);
        }
    }
}

void HookedLevelBrowserLayer::updatePlayerCounts(float) {
    auto& nm = NetworkManager::get();
    if (nm.established()) {
        std::vector<LevelId> levelIds;
        for (auto* cell_ : CCArrayExt<CCNode*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
            if (auto* cell = typeinfo_cast<LevelCell*>(cell_)) {
                if (isValidLevelType(cell->m_level->m_levelType)) {
                    levelIds.push_back(cell->m_level->m_levelID);
                }
            }
        }

        nm.send(RequestPlayerCountPacket::create(std::move(levelIds)));
    }
}

void HookedLevelBrowserLayer::destructor() {
    LevelBrowserLayer::~LevelBrowserLayer();

    auto& nm = NetworkManager::get();
    nm.removeListener<LevelPlayerCountPacket>(util::time::seconds(3));
}