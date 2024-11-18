#include "userlist.hpp"

#include "user_cell.hpp"
#include <audio/voice_playback_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <ui/general/slider.hpp>
#include <util/ui.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;
using namespace asp::time;

bool GlobedUserListPopup::setup() {
    this->setTitle("Players");

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    m_noElasticity = true;

    Build(UserList::createForComments(LIST_WIDTH, LIST_HEIGHT, GlobedUserCell::CELL_HEIGHT))
        .anchorPoint(0.5f, 1.f)
        .pos(rlayout.fromTop(45.f))
        .with([&](auto* list) {
            list->setCellColors(globed::color::DarkBrown, globed::color::Brown);
        })
        .parent(m_mainLayer)
        .store(listLayer);

    this->hardRefresh();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->hardRefresh();
        })
        .pos(rlayout.fromBottomRight(8.f, 8.f))
        .id("reload-btn"_spr)
        .parent(m_buttonMenu);

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
        .pos(rlayout.fromBottomLeft(20.f, 20.f))
        .id("report-btn"_spr)
        .parent(m_buttonMenu);

    // checkbox to toggle voice sorting
    auto* cbLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(rlayout.fromBottom(20.f))
        .parent(m_mainLayer)
        .collect();

    Build<BetterSlider>::create()
        .scale(0.85f)
        .id("volume-slider")
        .store(volumeSlider);

    volumeSlider->setContentWidth(80.f);
    volumeSlider->setLimits(0.0, 2.0);
    volumeSlider->setValue(GlobedSettings::get().communication.voiceVolume);
    volumeSlider->setCallback([this](BetterSlider* slider, double value) {
        this->onVolumeChanged(slider, value);
    });

    Build<CCLabelBMFont>::create("Voice Volume", "bigFont.fnt")
        .scale(0.45f * 0.7f)
        .intoNewParent(CCNode::create())
        .id("volume-wrapper")
        .child(volumeSlider)
        .layout(ColumnLayout::create()
            ->setAutoScale(false)
            ->setAxisReverse(true))
        .contentSize(80.f, 30.f)
        .anchorPoint(0.5f, 0.5f)
        .pos(rlayout.topRight - CCPoint{50.f, 25.f})
        .parent(m_mainLayer)
        .updateLayout();

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

    auto now = SystemTime::now();
    std::sort(cellIndices.begin(), cellIndices.end(), [&vpm, &cellMap, now = now](int idx1, int idx2) -> bool {
        constexpr auto limit = Duration::fromSecs(5);

        auto time1 = vpm.getLastPlaybackTime(idx1);
        auto time2 = vpm.getLastPlaybackTime(idx2);

        auto diff1 = now.durationSince(time1).value_or(Duration{});
        auto diff2 = now.durationSince(time2).value_or(Duration{});

        // if both users are speaking in the last 10 seconds, sort by who is more recent
        if (diff1 < limit && diff2 < limit) {
            auto secs1 = diff1.seconds();
            auto secs2 = diff2.seconds();

            // if there's a 0 second difference, sort by playername
            if (secs1 != secs2) {
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
    auto& players = playLayer->m_fields->players;
    auto& playerStore = playLayer->m_fields->playerStore;

    auto& pcm = ProfileCacheManager::get();
    auto& ownData = pcm.getOwnAccountData();

    std::vector<int> playerIds;

    // we are always first
    playerIds.push_back(ownData.accountId);

    for (const auto& [playerId, _] : players) {
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
        auto entry = playerStore->get(playerId).value_or(PlayerStore::Entry {
            .attempts = 0,
            .localBest = 0,
        });

        GlobedUserCell* cell;
        if (playerId == ownData.accountId) {
            cell = GlobedUserCell::create(entry, ownData, this);
        } else if (auto pcmdata = pcm.getData(playerId)) {
            cell = GlobedUserCell::create(entry, pcmdata.value(), this);
        } else {
            // cell = GlobedUserCell::create(entry, PlayerAccountData::DEFAULT_DATA, this);
            cell = nullptr;
        }

        if (cell) {
            cells->addObject(cell);
        }
    }

    return cells;
}

void GlobedUserListPopup::onToggleVoiceSort(cocos2d::CCObject* sender) {
    volumeSortEnabled = !static_cast<CCMenuItemToggler*>(sender)->isOn();
}

void GlobedUserListPopup::onVolumeChanged(BetterSlider* slider, double value) {
    GlobedSettings::get().communication.voiceVolume = value;
}

void GlobedUserListPopup::removeListCell(GlobedUserCell* cell) {
    listLayer->removeCell(cell);
}

GlobedUserListPopup* GlobedUserListPopup::create() {
    auto ret = new GlobedUserListPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}