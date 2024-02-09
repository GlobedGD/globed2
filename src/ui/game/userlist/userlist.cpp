#include "userlist.hpp"

#include "user_cell.hpp"
#include <hooks/play_layer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedUserListPopup::setup() {
    this->setTitle("Players");

    m_noElasticity = true;

    Build<GJCommentListLayer>::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false)
        .pos((m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2, 50.f)
        .parent(m_mainLayer)
        .store(listLayer);

    this->hardRefresh();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->hardRefresh();
        })
        .pos(m_size.width / 2.f - 3.f, -m_size.height / 2.f + 3.f)
        .id("reload-btn"_spr)
        .intoNewParent(CCMenu::create())
        .parent(m_mainLayer);

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
    float lastScrollPos = -1.f;
    if (listLayer->m_list) {
        lastScrollPos = util::ui::getScrollPos(listLayer->m_list);
        listLayer->m_list->removeFromParent();
    }

    listLayer->m_list = Build<ListView>::create(createPlayerCells(), GlobedUserCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT)
        .parent(listLayer)
        .collect();

    if (lastScrollPos != -1.f) {
        util::ui::setScrollPos(listLayer->m_list, lastScrollPos);
    }
}

CCArray* GlobedUserListPopup::createPlayerCells() {
    auto cells = CCArray::create();

    auto playLayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    auto& playerStore = playLayer->m_fields->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();
    auto& ownData = pcm.getOwnAccountData();

    std::vector<int> playerIds;

    for (const auto& [playerId, _] : playerStore) {
        playerIds.push_back(playerId);
    }

    auto& flm = FriendListManager::get();
    std::sort(playerIds.begin(), playerIds.end(), [&flm, &pcm, &playerStore](const auto& p1, const auto& p2) -> bool {
        bool isFriend1 = flm.isFriend(p1);
        bool isFriend2 = flm.isFriend(p2);

        if (isFriend1 != isFriend2) {
            return isFriend1;
        } else {
            auto accData1 = pcm.getData(p1);
            auto accData2 = pcm.getData(p2);
            if (!accData1 || !accData2) return false;
            // convert both names to lowercase
            std::string name1 = accData1.value().name, name2 = accData2.value().name;
            std::transform(name1.begin(), name1.end(), name1.begin(), ::tolower);
            std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);

            // sort alphabetically
            return name1 < name2;
        }
    });

    for (const auto playerId : playerIds) {
        auto& entry = playerStore.at(playerId);

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