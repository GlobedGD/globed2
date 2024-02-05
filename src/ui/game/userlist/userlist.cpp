#include "userlist.hpp"

#include "user_cell.hpp"
#include <hooks/play_layer.hpp>
#include <managers/profile_cache.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedUserListPopup::setup() {
    this->setTitle("Players");

    Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .store(listLayer);

    this->schedule(schedule_selector(GlobedUserListPopup::reloadList), 0.1f);

    this->hardRefresh();

    return true;
}

void GlobedUserListPopup::reloadList(float) {
    auto playLayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    auto& playerStore = playLayer->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();

    size_t existingCount = listLayer->m_list->m_entries->count();

    // if the count is different, hard refresh
    if (playerStore.size() != existingCount) {
        this->hardRefresh();
        return;
    }

    size_t refreshed = 0;
    for (auto* cell : CCArrayExt<GlobedUserCell*>(listLayer->m_list->m_entries)) {
        for (const auto& [playerId, entry] : playerStore) {
            if (playerId == cell->playerId) {
                cell->refreshData(entry);
                refreshed++;
            }
        }
    }

    // if fails, do hard refresh
    if (refreshed != existingCount) {
        this->hardRefresh();
    }
}

void GlobedUserListPopup::hardRefresh() {
    if (listLayer->m_list) listLayer->m_list->removeFromParent();

    listLayer->m_list = Build<ListView>::create(createPlayerCells())
        .parent(listLayer)
        .collect();
}

CCArray* GlobedUserListPopup::createPlayerCells() {
    auto cells = CCArray::create();

    auto playLayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    auto& playerStore = playLayer->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();

    for (const auto& [playerId, entry] : playerStore) {
        auto data = pcm.getData(playerId);
        if (!data) continue;

        cells->addObject(GlobedUserCell::create(entry, data->name));
    }

    return cells;
}

GlobedUserListPopup* GlobedUserListPopup::create() {
    auto ret = new GlobedUserListPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}