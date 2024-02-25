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
    for (auto* level : CCArrayExt<GJGameLevel*>(p0)) {
        if (level->m_levelType >= GJLevelType::Saved) {
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
    for (auto* cell : CCArrayExt<LevelCell*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
        if (cell->m_level->m_levelType < GJLevelType::Saved) continue;

        if (m_fields->levels.contains(cell->m_level->m_levelID)) {
            static_cast<GlobedLevelCell*>(cell)->updatePlayerCount(m_fields->levels.at(cell->m_level->m_levelID));
        } else {
            static_cast<GlobedLevelCell*>(cell)->updatePlayerCount(-1);
        }
    }
}

void HookedLevelBrowserLayer::updatePlayerCounts(float) {
    auto& nm = NetworkManager::get();
    if (nm.established()) {
        std::vector<LevelId> levelIds;
        for (auto* cell : CCArrayExt<LevelCell*>(m_list->m_listView->m_tableView->m_contentLayer->getChildren())) {
            if (cell->m_level->m_levelType >= GJLevelType::Saved) {
                levelIds.push_back(cell->m_level->m_levelID);
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