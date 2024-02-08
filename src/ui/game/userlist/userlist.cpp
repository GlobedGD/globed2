#include "userlist.hpp"

#include "user_cell.hpp"
#include <hooks/play_layer.hpp>
#include <managers/profile_cache.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedUserListPopup::setup() {
    this->setTitle("Players");

    m_noElasticity = true;

    Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .pos((m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2, 50.f)
        .parent(m_mainLayer)
        .store(listLayer);

    this->schedule(schedule_selector(GlobedUserListPopup::reloadList), 0.1f);

    this->hardRefresh();

    return true;
}

void GlobedUserListPopup::reloadList(float) {
    auto playLayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (!playLayer) return;

    auto& playerStore = playLayer->m_fields->playerStore->getAll();
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

    listLayer->m_list = Build<ListView>::create(createPlayerCells(), GlobedUserCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();
}

CCArray* GlobedUserListPopup::createPlayerCells() {
    auto cells = CCArray::create();

    auto playLayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    auto& playerStore = playLayer->m_fields->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();
    auto& ownData = pcm.getOwnAccountData();

    for (const auto& [playerId, entry] : playerStore) {
        GlobedUserCell* cell;
        if (playerId == ownData.id) {
            cell = GlobedUserCell::create(entry, ownData);
        } else {
            auto pcmdata = pcm.getData(playerId);
            if (!pcmdata) continue;
            cell = GlobedUserCell::create(entry, pcmdata.value());
        }

        cells->addObject(cell);
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