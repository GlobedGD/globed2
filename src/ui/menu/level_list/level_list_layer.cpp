#include "level_list_layer.hpp"

#include <globed/tracing.hpp>
#include <data/packets/server/general.hpp>
#include <data/packets/client/general.hpp>
#include <hooks/gjgamelevel.hpp>
#include <hooks/level_cell.hpp>
#include <net/manager.hpp>
#include <managers/error_queues.hpp>
#include <util/ui.hpp>
#include <util/math.hpp>

#include "filters_popup.hpp"



using namespace geode::prelude;

// ok so let's say we load the layer
// request all levelsfrom the server, get a list of a ton of level ids, pre-emptively sort it by player counts
//
// at a time, request up to 100 levels from the server, then do client side filtering if any filters are enabled.
// if not enough levels to fill a page, keep making requests.


static std::string rateTierToString(GlobedLevelListLayer::Filters::RateTier tier) {
    using enum GlobedLevelListLayer::Filters::RateTier;
    switch (tier) {
        case Unrated: return "Unrated";
        case Rate: return "Rate";
        case Feature: return "Feature";
        case Epic: return "Epic";
        case Legendary: return "Legendary";
        case Mythic: return "Mythic";
    }
}

static std::optional<GlobedLevelListLayer::Filters::RateTier> rateTierFromString(std::string_view str) {
    using enum GlobedLevelListLayer::Filters::RateTier;
    if (str == "Unrated") return Unrated;
    if (str == "Rate") return Rate;
    if (str == "Feature") return Feature;
    if (str == "Epic") return Epic;
    if (str == "Legendary") return Legendary;
    if (str == "Mythic") return Mythic;
    return std::nullopt;
}

template <>
struct matjson::Serialize<GlobedLevelListLayer::Filters> {
    static Result<GlobedLevelListLayer::Filters> fromJson(const matjson::Value& value) {
        GlobedLevelListLayer::Filters filters{};

        if (value["difficulty"].isArray()) {
            for (auto& diff : value["difficulty"].asArray().unwrap()) {
                auto diffe = util::gd::difficultyFromString(GEODE_UNWRAP(diff.asString()));
                if (diffe) {
                    filters.difficulty.insert(diffe.value());
                }
            }
        }

        if (value["demonDifficulty"].isArray()) {
            for (auto& diff : value["demonDifficulty"].asArray().unwrap()) {
                auto diffe = util::gd::difficultyFromString(GEODE_UNWRAP(diff.asString()));
                if (diffe) {
                    filters.demonDifficulty.insert(diffe.value());
                }
            }
        }

        if (value["length"].isArray()) {
            for (auto& len : value["length"].asArray().unwrap()) {
                filters.length.insert(GEODE_UNWRAP(len.asInt()));
            }
        }

        if (value["rateTier"].isArray()) {
            filters.rateTier = std::set<GlobedLevelListLayer::Filters::RateTier>{};
            for (auto& tier : value["rateTier"].asArray().unwrap()) {
                auto rt = rateTierFromString(GEODE_UNWRAP(tier.asString()));
                if (rt) {
                    filters.rateTier.value().insert(rt.value());
                }
            }
        }

        if (value["coins"].isBool()) {
            filters.coins = value["coins"].asBool().unwrap();
        }

        if (value["completed"].isBool()) {
            filters.completed = value["completed"].asBool().unwrap();
        }

        if (value.contains("twoPlayer") && value["twoPlayer"].isBool()) {
            filters.twoPlayer = value["twoPlayer"].asBool().unwrap();
        }

        if (value.contains("rated") && value["rated"].isBool()) {
            filters.rated = value["rated"].asBool().unwrap();
        }

        if (value.contains("original") && value["original"].isBool()) {
            filters.original = value["original"].asBool().unwrap();
        }

        return Ok(filters);
    }

    static matjson::Value toJson(const GlobedLevelListLayer::Filters& filters) {
        matjson::Value obj;
        obj["difficulty"] = matjson::Value::array();
        for (auto diff : filters.difficulty) {
            obj["difficulty"].asArray().unwrap().push_back(util::gd::difficultyToString(diff));
        }

        obj["demonDifficulty"] = matjson::Value::array();
        for (auto diff : filters.demonDifficulty) {
            obj["demonDifficulty"].asArray().unwrap().push_back(util::gd::difficultyToString(diff));
        }

        obj["length"] = matjson::Value::array();
        for (auto len : filters.length) {
            obj["length"].asArray().unwrap().push_back(len);
        }

        if (auto& rt = filters.rateTier) {
            obj["rateTier"] = matjson::Value::array();
            for (auto tier : rt.value()) {
                obj["rateTier"].asArray().unwrap().push_back(rateTierToString(tier));
            }
        }

        if (auto& c = filters.coins) {
            obj["coins"] = *c;
        }

        if (auto& c = filters.completed) {
            obj["completed"] = *c;
        }

        obj["twoPlayer"] = filters.twoPlayer;
        obj["rated"] = filters.rated;
        obj["original"] = filters.original;

        return obj;
    }
};

bool GlobedLevelListLayer::init() {
    if (!CCLayer::init()) return false;

    auto& settings = GlobedSettings::get();
    this->pageSize = settings.globed.increaseLevelList ? INCREASED_LIST_PAGE_SIZE : LIST_PAGE_SIZE;

    auto winSize = CCDirector::get()->getWinSize();

    // side art and bg and stuff
    geode::addSideArt(this, SideArt::Bottom);
    util::ui::prepareLayer(this);

    Build<GJListLayer>::create(nullptr, "Levels", globed::color::Brown, LIST_WIDTH, LIST_HEIGHT, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .id("level-list"_spr)
        .store(listLayer);

    listLayer->setPosition(winSize / 2 - listLayer->getScaledContentSize() / 2);

    auto menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .zOrder(2)
        .parent(this)
        .collect();

    // refresh button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->onRefresh();
        })
        .pos(winSize.width - 32.f, 32.f)
        .parent(menu)
        .store(btnRefresh);

    // filter button
    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            this->onOpenFilters();
        })
        .pos(25.f, winSize.height - 70.f)
        .parent(menu)
        .store(btnFilters);

    // buttons to switch pages

    constexpr float pageBtnPadding = 20.f;

    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .intoMenuItem([this](auto) {
            this->onPrevPage();
        })
        .pos(pageBtnPadding, winSize.height / 2)
        .store(btnPagePrev)
        .parent(menu);

    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .with([&](auto spr) { spr->setFlipX(true); })
        .intoMenuItem([this](auto) {
            this->onNextPage();
        })
        .pos(winSize.width - pageBtnPadding, winSize.height / 2)
        .store(btnPageNext)
        .parent(menu);

    // loading circle
    Build<BetterLoadingCircle>::create(true)
        .zOrder(11)
        .store(loadingCircle)
        .parent(listLayer)
        .pos(listLayer->getScaledContentSize() / 2.f);

    DailyManager::get().getCurrentLevelMeta([this](const GlobedFeaturedLevel& meta) {
        currentFeaturedLevel = meta;
    });

    NetworkManager::get().addListener<LevelListPacket>(this, [this](std::shared_ptr<LevelListPacket> packet) {
        if (GlobedSettings::get().launchArgs().fakeData) {
            std::initializer_list<GlobedLevel> levels = {
                {110715909, 23},
                {110681124, 52},
                {27732941, 2},
                {110774330, 12},
                {110774310, 15},
                {110638716, 44},
                {110772605, 1},
                {110705309, 58},
                {110517732, 7},
                {110418122, 9},
                {99923697, 10},
                {110774148, 85},
                {110290111, 23},
                {110719349, 15},
                {110714865, 81},
                {110625662, 92},
                {110610038, 3},
                {110594994, 9},
                {110512795, 1},
                {110500920, 97},
                {110452453, 1},
                {110428166, 443},
                {110430434, 23},
                {110873135, 12412},
                {110873134, 291},
                {110873130, 12},
                {108789649, 151},
                {103632860, 59},
                {100496253, 1958},
                {108447741, 12},
                {123, 45},
            };

            packet->levels = levels;
        }

        this->playerCounts.clear();
        this->allLevelIds.clear();
        this->currentQuery.clear();

        for (const auto& level : packet->levels) {
            if (util::misc::isEditorCollabLevel(level.levelId)) continue;

            this->playerCounts.emplace(level.levelId, level.playerCount);
            this->allLevelIds.push_back(level.levelId);
        }

        std::sort(allLevelIds.begin(), allLevelIds.end(), [&](LevelId a, LevelId b) {
            if (!this->playerCounts.contains(a)) {
                return false;
            }

            if (!this->playerCounts.contains(b)) {
                return true;
            }

            return playerCounts.at(a) > playerCounts.at(b);
        });

        this->currentPage = 0;
        this->startLoadingForPage();
    });

    if (auto filtersJson = Mod::get()->getSaveContainer()["saved-level-filters"].as<GlobedLevelListLayer::Filters>()) {
        this->filters = *filtersJson;
    }

    this->onRefresh();

    return true;
}

GlobedLevelListLayer::~GlobedLevelListLayer() {
    GameLevelManager::get()->m_levelManagerDelegate = nullptr;
}

void GlobedLevelListLayer::keyBackClicked() {
    util::ui::navigateBack();
}

void GlobedLevelListLayer::onOpenFilters() {
    GlobedLevelFilterPopup::create(this->filters, this, [this](auto filters) {
        if (filters != this->filters) {
            this->filters = filters;
            this->currentPage = 0;
            this->startLoadingForPage();

            matjson::Value val = filters;
            Mod::get()->setSavedValue("saved-level-filters", val);
        }
    })->show();
}

void GlobedLevelListLayer::onRefresh() {
    if (loading) return;

    loading = true;

    auto& nm = NetworkManager::get();
    if (!nm.established()) return;

    // request the level list from the server
    nm.send(RequestLevelListPacket::create());

    this->showLoadingUi();
}

void GlobedLevelListLayer::onNextPage() {
    currentPage = util::math::min(currentPage + 1, allLevelIds.size() / pageSize); // TODO idk if this is correcto
    this->startLoadingForPage();
}

void GlobedLevelListLayer::onPrevPage() {
    currentPage = util::math::max(currentPage - 1, 0);
    this->startLoadingForPage();
}

void GlobedLevelListLayer::startLoadingForPage() {
    loading = true;

    TRACE("Start loading for page {}", currentPage);

    this->showLoadingUi();

    // if no levels, don't make a request
    if (allLevelIds.empty()) {
        this->finishLoading();
        return;
    }

    this->continueLoading();
}

void GlobedLevelListLayer::continueLoading() {
    // ok so we have a vector of all level ids
    // a level can be
    // * cached - present in levelCache, no need to fetch, does count as a level IF matches filters
    // * ignored - present in failedQueries, one of the previous queries did not return this level, no need to fetch, does not count as a level
    // * unknown - not present anywhere, does count as a level

    // first check if we have enough levels to display.
    size_t requiredCount = (1 + currentPage) * pageSize;
    size_t hasCount = 0;

    for (auto id : allLevelIds) {
        if (levelCache.contains(id)) {
            if (this->isMatchingFilters(levelCache[id])) {
                hasCount++;
            }
        }
    }

    TRACE("Continue loading, count = {} / {}", hasCount, requiredCount);

    if (hasCount >= requiredCount || !loadNextBatch()) {
        // either we already have enough levels, or there's just not enough at all.
        // simply halt.
        this->finishLoading();
    }
}

void GlobedLevelListLayer::finishLoading() {
    std::vector<Ref<GJGameLevel>> page;

    size_t counter = 0;
    size_t unloadedLevels = 0; // levels that haven't been loaded yet
    size_t reqMin = currentPage * pageSize;

    for (auto id : allLevelIds) {
        if (levelCache.contains(id)) {
            if (this->isMatchingFilters(levelCache[id])) {
                counter++;

                if (counter > reqMin && page.size() < pageSize) {
                    page.push_back(levelCache[id]);
                }
            }
        } else if (!failedQueries.contains(id)) {
            unloadedLevels++;
        }
    }

    TRACE("Finished loading, page size = {}, counter = {}, unloaded = {}, reqm = {}, failed = {}", page.size(), counter, unloadedLevels, reqMin, failedQueries.size());

    // TODO: idk if unloaded levels thing is rightt
    bool showNextPage = counter > (reqMin + pageSize) || unloadedLevels;

    // sort by player count descending
    std::sort(page.begin(), page.end(), [&](GJGameLevel* a, GJGameLevel* b) {
        LevelId aid = HookedGJGameLevel::getLevelIDFrom(a);
        LevelId bid = HookedGJGameLevel::getLevelIDFrom(b);

        if (!this->playerCounts.contains(aid)) {
            return false;
        }

        if (!this->playerCounts.contains(bid)) {
            return true;
        }

        return playerCounts.at(aid) > playerCounts.at(bid);
    });

    CCArray* finalArray = CCArray::create();
    for (auto level : page) {
        finalArray->addObject(level);
    }

    if (listLayer->m_listView) listLayer->m_listView->removeFromParent();
    listLayer->m_listView = Build<CustomListView>::create(finalArray, BoomListType::Level, LIST_HEIGHT, LIST_WIDTH)
        .parent(listLayer)
        .collect();

    // guys we are about to do a funny
    for (LevelCell* cell : CCArrayExt<LevelCell*>(listLayer->m_listView->m_tableView->m_contentLayer->getChildren())) {
        int levelId = HookedGJGameLevel::getLevelIDFrom(cell->m_level);
        if (!playerCounts.contains(levelId)) continue;

        auto globedCell = static_cast<GlobedLevelCell*>(cell);
        globedCell->updatePlayerCount(playerCounts.at(levelId));
        if (globedCell->m_level->m_levelID == currentFeaturedLevel.levelId) {
            globedCell->modifyToFeaturedCell(currentFeaturedLevel.rateTier);
            globedCell->m_fields->rateTier = currentFeaturedLevel.rateTier;
        }
    }

    // show the buttons
    if (currentPage > 0) {
        btnPagePrev->setVisible(true);
    }

    if (showNextPage) {
        btnPageNext->setVisible(true);
    }

    btnRefresh->setVisible(true);
    btnFilters->setVisible(true);

    loadingCircle->fadeOut();

    loading = false;
}

bool GlobedLevelListLayer::loadNextBatch() {
    currentQuery.clear();

    for (auto id : allLevelIds) {
        if (levelCache.contains(id) || failedQueries.contains(id)) continue;

        currentQuery.push_back(id);

        if (currentQuery.size() == 100) {
            break;
        }
    }

    if (currentQuery.empty()) {
        return false;
    }

    std::string query = fmt::format("{}", fmt::join(currentQuery, ","));

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;

    auto sobj = GJSearchObject::create(SearchType::Type26, query);
    glm->getOnlineLevels(sobj);

    return true;
}

bool GlobedLevelListLayer::isMatchingFilters(GJGameLevel* level) {
    using enum Filters::RateTier;
    using enum util::gd::Difficulty;
    using Difficulty = util::gd::Difficulty;

    if (!level) return false;

    auto difficulty = util::gd::calcLevelDifficulty(level);

    // log::debug("Name = {}, diff = {}, demon diff = {}, is demon = {}, length = {}, epic = {}, featured = {}, stars = {}, coins = {}", level->m_levelName, (int) difficulty, (int)level->m_demonDifficulty, (int)level->m_demon, (int)level->m_levelLength, level->m_isEpic, level->m_featured, (int)level->m_stars, level->m_coins);
    // return true;

    // Difficulty
    if (!filters.difficulty.empty()) {
        auto difficulty2 = difficulty;

        // convert to HardDemon for this check
        if (difficulty == EasyDemon || difficulty == MediumDemon || difficulty == InsaneDemon || difficulty == ExtremeDemon) {
            difficulty2 = HardDemon;
        }

        // non demon filtering
        if (std::find(filters.difficulty.begin(), filters.difficulty.end(), difficulty2) == filters.difficulty.end()) {
            return false;
        }

        // demon filtering
        if (difficulty2 == HardDemon) {
            if (!level->m_demon) return false;

            if (!filters.demonDifficulty.empty()) {
                // check for specific demon difficulty
                if (std::find(filters.demonDifficulty.begin(), filters.demonDifficulty.end(), difficulty) == filters.demonDifficulty.end()) {
                    return false;
                }
            }
        }
    }

    if (!filters.length.empty()) {
        if (std::find(filters.length.begin(), filters.length.end(), level->m_levelLength) == filters.length.end()) {
            return false;
        }
    }

    if (filters.rateTier) {
        auto& tiers = filters.rateTier.value();

        Filters::RateTier rateTier;

        if (level->m_isEpic == 3) {
            rateTier = Mythic;
        } else if (level->m_isEpic == 2) {
            rateTier = Legendary;
        } else if (level->m_isEpic == 1) {
            rateTier = Epic;
        } else if (level->m_featured > 0) {
            rateTier = Feature;
        } else if (level->m_stars > 0) {
            rateTier = Rate;
        } else {
            rateTier = Unrated;
        }

        if (std::find(tiers.begin(), tiers.end(), rateTier) == tiers.end()) {
            return false;
        }
    }

    if (filters.coins) {
        if (*filters.coins != (level->m_coins > 0)) {
            return false;
        }
    }

    if (filters.completed) {
        bool hasCompleted = level->m_dailyID > 0 ? level->m_orbCompletion > 99 : GameStatsManager::sharedState()->hasCompletedLevel(level);

        if (*filters.completed != hasCompleted) {
            return false;
        }
    }

    if (filters.twoPlayer && !level->m_twoPlayerMode) {
        return false;
    }

    if (filters.rated && level->m_stars <= 0) {
        return false;
    }

    // if (filters.song) {
    //     log::debug("Song id = {}, ids = {}", level->m_songID, level->m_songIDs);
    //     // TODO
    // }

    // if (filters.verifiedCoins && !level->m_coinsVerified) {
    //     return false;
    // }

    if (filters.original && level->m_originalLevel != 0) {
        return false;
    }

    return true;
}

void GlobedLevelListLayer::showLoadingUi() {
    btnPagePrev->setVisible(false);
    btnPageNext->setVisible(false);
    btnRefresh->setVisible(false);
    btnFilters->setVisible(false);

    loadingCircle->fadeIn();

    // remove list view and set to null
    if (listLayer->m_listView) listLayer->m_listView->removeFromParent();
    listLayer->m_listView = nullptr;
}

void GlobedLevelListLayer::loadLevelsFinished(CCArray* arr, char const* q) {
    this->loadLevelsFinished(arr, q, -1);
}

void GlobedLevelListLayer::loadLevelsFinished(CCArray* arr, char const*, int) {
    for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(arr)) {
        levelCache[level->m_levelID] = level;
    }

    // check if there are any levels not present, add them to the failed list
    for (auto id : currentQuery) {
        if (!levelCache.contains(id)) {
            TRACE("Invalid level: {}", id);
            failedQueries.insert(id);
        }
    }

    this->continueLoading();
}

void GlobedLevelListLayer::loadLevelsFailed(char const* q) {
    this->loadLevelsFailed(q, -1);
}

void GlobedLevelListLayer::loadLevelsFailed(char const* q, int p) {
    log::warn("query failed ({}): {}", p, q);
    // ErrorQueues::get().warn(fmt::format("Failed to load levels: error {}", p));

    this->finishLoading();
}

GlobedLevelListLayer* GlobedLevelListLayer::create() {
    auto ret = new GlobedLevelListLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}