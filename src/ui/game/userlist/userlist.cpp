#include "userlist.hpp"

#include "user_cell.hpp"
#include <audio/voice_playback_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool GlobedUserListPopup::setup() {
    this->setTitle("Players");

    auto rlayout = util::ui::getPopupLayout(m_size);

    m_noElasticity = true;

    Build(UserList::createForComments(LIST_WIDTH, LIST_HEIGHT, GlobedUserCell::CELL_HEIGHT))
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(40.f))
        .with([&](auto* list) {
            list->setCellColors(util::ui::BG_COLOR_DARKBROWN, util::ui::BG_COLOR_BROWN);
        })
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

    Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
        .scale(0.5f)
        .intoMenuItem([this](auto) {
            createQuickPopup(
                "Reporting",
                "Someone <cr>breaking</c> the <cl>Globed</c> rules?\n<cg>Join</c> the <cl>Globed</c> discord and <cr>report</c> them there!",
                "Cancel",
                "Join",
                [this](auto, bool btn2) {
                    if (btn2) {
                        geode::utils::web::openLinkInBrowser(globed::string<"discord">());
                    }
                }
            );
        })
        .pos(-m_size.width / 2.f + 20.f, -m_size.height / 2.f + 22.f)
        .id("report-btn"_spr)
        .intoNewParent(CCMenu::create())
        .parent(m_mainLayer);

    // checkbox to toggle voice sorting
    auto* cbLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(rlayout.centerBottom + CCPoint{0.f, 20.f})
        .parent(m_mainLayer)
        .collect();


    volumeSlider = Build<Slider>::create(this, menu_selector(GlobedUserListPopup::onVolumeChanged))
        .pos(rlayout.topRight + ccp(-75, -30 - 7) * 0.7f + ccp(0, -5))
        .anchorPoint(0, 0)
        .scale(0.45f * 0.7f)
        //.scaleX(0.35f)
        .parent(m_mainLayer)
        .value(GlobedSettings::get().communication.voiceVolume / 2)
        .collect();

    Build<CCLabelBMFont>::create("Voice Volume", "bigFont.fnt")
        .pos(rlayout.topRight + ccp(-75, -18) * 0.7f + ccp(0, -5))
        .scale(0.45f * 0.7f)
        .parent(m_mainLayer);

    // Build<CCMenuItemToggler>(CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GlobedUserListPopup::onToggleVoiceSort), 0.7f))
    //     .id("toggle-voice-sort"_spr)
    //     .parent(cbLayout);

    // Build<CCLabelBMFont>::create("Sort by voice", "bigFont.fnt")
    //     .scale(0.4f)
    //     .id("toggle-voice-sort-hint"_spr)
    //     .parent(cbLayout);

    cbLayout->updateLayout();

    // this->schedule(schedule_selector(GlobedUserListPopup::reorderWithVolume), 0.5f);

    return true;
}

void GlobedUserListPopup::reorderWithVolume(float) {
    if (!volumeSortEnabled) return;

    auto& vpm = VoicePlaybackManager::get();

    std::unordered_map<int, GlobedUserCell*> cellMap;
    std::vector<int> cellIndices;

    for (auto* entry : *listLayer) {
        cellMap[entry->accountData.accountId] = entry;
        cellIndices.push_back(entry->accountData.accountId);
    }

    auto now = util::time::now();
    std::sort(cellIndices.begin(), cellIndices.end(), [&vpm, &cellMap, now = now](int idx1, int idx2) -> bool {
        constexpr util::time::seconds limit(5);

        auto time1 = vpm.getLastPlaybackTime(idx1);
        auto time2 = vpm.getLastPlaybackTime(idx2);

        auto diff1 = now - time1;
        auto diff2 = now - time2;

        // if both users are speaking in the last 10 seconds, sort by who is more recent
        if (diff1 < limit && diff2 < limit) {
            auto secs1 = util::time::asSeconds(diff1);
            auto secs2 = util::time::asSeconds(diff2);

            // if there's a 0 second difference, sort by playername
            if (std::abs(secs2 - secs1) == 0) {
                return util::misc::compareName(cellMap[idx1]->accountData.name, cellMap[idx2]->accountData.name);
            }

            // otherwise sort by recently speaking
            return diff1 < diff2;
        } else if (diff1 < limit) {
            return true;
        } else if (diff2 < limit) {
            return false;
        }

        // if both users haven't spoken, sort by name
        return util::misc::compareName(cellMap[idx1]->accountData.name, cellMap[idx2]->accountData.name);
    });

    auto sorted = CCArray::create();
    for (int idx : cellIndices) {
        sorted->addObject(cellMap[idx]);
    }

    listLayer->swapCells(sorted);

    vpm.getLastPlaybackTime(0);
}

void GlobedUserListPopup::reloadList(float) {
    auto playLayer = GlobedGJBGL::get();
    if (!playLayer) return;

    auto& playerStore = playLayer->m_fields->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();

    size_t existingCount = listLayer->cellCount();

    // if the count is different, hard refresh
    if (playerStore.size() != existingCount) {
        this->hardRefresh();
        return;
    }

    size_t refreshed = 0;
    for (auto* cell : *listLayer) {
        for (const auto& [playerId, entry] : playerStore) {
            if (playerId == cell->accountData.accountId) {
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
    listLayer->swapCells(createPlayerCells());
}

CCArray* GlobedUserListPopup::createPlayerCells() {
    auto cells = CCArray::create();

    auto playLayer = GlobedGJBGL::get();
    auto& playerStore = playLayer->m_fields->playerStore->getAll();
    auto& pcm = ProfileCacheManager::get();
    auto& ownData = pcm.getOwnAccountData();

    std::vector<int> playerIds;

    for (const auto& [playerId, _] : playerStore) {
        playerIds.push_back(playerId);
    }

    auto& flm = FriendListManager::get();
    std::sort(playerIds.begin(), playerIds.end(), [&flm, &pcm, &playerStore](const auto& p1, const auto& p2) -> bool {
        auto selfId = GJAccountManager::get()->m_accountID;

        if (p1 == selfId) return true;
        else if (p2 == selfId) return false;

        bool isFriend1 = flm.isFriend(p1);
        bool isFriend2 = flm.isFriend(p2);

        if (isFriend1 != isFriend2) {
            return isFriend1;
        } else {
            auto accData1 = pcm.getData(p1);
            auto accData2 = pcm.getData(p2);
            if (!accData1 || !accData2) return false;

            return util::misc::compareName(accData1.value().name, accData2.value().name);
        }
    });

    this->setTitle(fmt::format("Players ({})", playerIds.size()));

    for (const auto playerId : playerIds) {
        auto& entry = playerStore.at(playerId);

        GlobedUserCell* cell;
        if (playerId == ownData.accountId) {
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

void GlobedUserListPopup::onToggleVoiceSort(cocos2d::CCObject* sender) {
    volumeSortEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
}

void GlobedUserListPopup::onVolumeChanged(cocos2d::CCObject* sender) {
    GlobedSettings::get().communication.voiceVolume = volumeSlider->getThumb()->getValue() * 2;
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