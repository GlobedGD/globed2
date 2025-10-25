#include "LevelListLayer.hpp"
#include "LevelFiltersPopup.hpp"
#include <globed/core/SettingsManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/LevelCell.hpp>
#include <ui/menu/FeatureCommon.hpp>

#include <UIBuilder.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

// ok so let's say we load the layer
// request all levelsfrom the server, get a list of a ton of level ids, pre-emptively sort it by player counts
//
// at a time, request up to 100 levels from the server, then do client side filtering if any filters are enabled.
// if not enough levels to fill a page, keep making requests.


static std::string rateTierToString(globed::LevelListLayer::Filters::RateTier tier) {
    using enum globed::LevelListLayer::Filters::RateTier;
    switch (tier) {
        case Unrated: return "Unrated";
        case Rate: return "Rate";
        case Feature: return "Feature";
        case Epic: return "Epic";
        case Legendary: return "Legendary";
        case Mythic: return "Mythic";
    }
}

static std::optional<globed::LevelListLayer::Filters::RateTier> rateTierFromString(std::string_view str) {
    using enum globed::LevelListLayer::Filters::RateTier;
    if (str == "Unrated") return Unrated;
    if (str == "Rate") return Rate;
    if (str == "Feature") return Feature;
    if (str == "Epic") return Epic;
    if (str == "Legendary") return Legendary;
    if (str == "Mythic") return Mythic;
    return std::nullopt;
}

template <>
struct matjson::Serialize<globed::LevelListLayer::Filters> {
    static Result<globed::LevelListLayer::Filters> fromJson(const matjson::Value& value) {
        globed::LevelListLayer::Filters filters{};

        if (value["difficulty"].isArray()) {
            for (auto& diff : value["difficulty"].asArray().unwrap()) {
                auto diffe = globed::difficultyFromString(GEODE_UNWRAP(diff.asString()));
                if (diffe) {
                    filters.difficulty.insert(diffe.value());
                }
            }
        }

        if (value["demonDifficulty"].isArray()) {
            for (auto& diff : value["demonDifficulty"].asArray().unwrap()) {
                auto diffe = globed::difficultyFromString(GEODE_UNWRAP(diff.asString()));
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
            filters.rateTier = std::set<globed::LevelListLayer::Filters::RateTier>{};
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

    static matjson::Value toJson(const globed::LevelListLayer::Filters& filters) {
        matjson::Value obj;
        obj["difficulty"] = matjson::Value::array();
        for (auto diff : filters.difficulty) {
            obj["difficulty"].asArray().unwrap().push_back(globed::difficultyToString(diff));
        }

        obj["demonDifficulty"] = matjson::Value::array();
        for (auto diff : filters.demonDifficulty) {
            obj["demonDifficulty"].asArray().unwrap().push_back(globed::difficultyToString(diff));
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

static std::vector<std::pair<globed::SessionId, uint16_t>> getFakeLevels();

namespace globed {

bool LevelListLayer::init() {
    if (!BaseLayer::init()) return false;

    m_pageSize = globed::setting<bool>("core.ui.increase-level-list") ? 100 : 30;

    auto winSize = CCDirector::get()->getWinSize();

    geode::addSideArt(this, SideArt::Bottom);

    m_list = Build(cue::ListNode::createLevels({358.f, 220.f}))
        .zOrder(2)
        .anchorPoint(0.5f, 0.5f)
        .pos(winSize / 2.f)
        .parent(this)
        .id("level-list"_spr);
    m_list->setAutoUpdate(false);

    auto menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .zOrder(2)
        .parent(this)
        .collect();


    // refresh button
    m_btnRefresh = Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->onRefresh();
        })
        .pos(winSize.width - 32.f, 32.f)
        .parent(menu);

    // filter button
    m_btnFilters = Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .scale(0.7f)
        .intoMenuItem([this](auto) {
            this->onOpenFilters();
        })
        .pos(25.f, winSize.height - 70.f)
        .parent(menu);

    // buttons to switch pages

    constexpr float pageBtnPadding = 20.f;

    m_btnPagePrev = Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .intoMenuItem([this](auto) {
            this->onPrevPage();
        })
        .pos(pageBtnPadding, winSize.height / 2)
        .parent(menu);

    m_btnPageNext = Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .with([&](auto spr) { spr->setFlipX(true); })
        .intoMenuItem([this](auto) {
            this->onNextPage();
        })
        .pos(winSize.width - pageBtnPadding, winSize.height / 2)
        .parent(menu);

    // loading circle
    m_loadingCircle = cue::LoadingCircle::create(true);
    m_loadingCircle->setZOrder(11);
    m_loadingCircle->addToLayer(m_list);

    if (auto filtersJson = globed::value<Filters>("core.ui.saved-level-filters")) {
        m_filters = *filtersJson;
    }

    m_levelListener = NetworkManagerImpl::get().listen<msg::LevelListMessage>([this](const auto& msg) {
        this->onLoaded(msg.levels);
        return ListenerResult::Continue;
    });

    this->onRefresh();

    return true;
}

LevelListLayer::~LevelListLayer() {
    GameLevelManager::get()->m_levelManagerDelegate = nullptr;
}

void LevelListLayer::onRefresh() {
    if (m_loading) return;

    m_loading = true;

    this->toggleLoadingUi(true);

    if (globed::setting<bool>("core.dev.fake-data")) {
        this->onLoaded(getFakeLevels());
    } else {
        NetworkManagerImpl::get().sendRequestLevelList();
    }
}

void LevelListLayer::onLoaded(const std::vector<std::pair<SessionId, uint16_t>>& levels) {
    m_playerCounts.clear();
    m_allLevelIds.clear();
    m_currentQuery.clear();

    for (const auto& level : levels) {
        m_playerCounts.emplace(level.first, level.second);
        m_allLevelIds.push_back(level.first);
    }

    std::sort(m_allLevelIds.begin(), m_allLevelIds.end(), [&](SessionId a, SessionId b) {
        if (!m_playerCounts.contains(a)) {
            return false;
        }

        if (!m_playerCounts.contains(b)) {
            return true;
        }

        return m_playerCounts.at(a) > m_playerCounts.at(b);
    });

    m_currentPage = 0;
    this->startLoadingForPage();
}

void LevelListLayer::onOpenFilters() {
    auto popup = LevelFiltersPopup::create(this);
    popup->setCallback([this](auto filters) {
        if (filters != m_filters) {
            m_filters = filters;
            m_currentPage = 0;
            this->startLoadingForPage();

            globed::setValue("core.ui.saved-level-filters", filters);
        }
    });
    popup->show();
}

void LevelListLayer::onNextPage() {
    m_currentPage = std::min<size_t>(m_currentPage + 1, m_allLevelIds.size() / m_pageSize); // TODO idk if this is correcto
    this->startLoadingForPage();
}

void LevelListLayer::onPrevPage() {
    m_currentPage = std::max<size_t>(m_currentPage - 1, 0);
    this->startLoadingForPage();
}

void LevelListLayer::startLoadingForPage() {
    m_loading = true;

    log::debug("Start loading for page {}", m_currentPage);

    this->toggleLoadingUi(true);

    // if no levels, don't make a request
    if (m_allLevelIds.empty()) {
        this->finishLoading();
        return;
    }

    this->continueLoading();
}

void LevelListLayer::continueLoading() {
    // ok so we have a vector of all level ids
    // a level can be
    // * cached - present in levelCache, no need to fetch, does count as a level IF matches filters
    // * ignored - present in failedQueries, one of the previous queries did not return this level, no need to fetch, does not count as a level
    // * unknown - not present anywhere, does count as a level

    // first check if we have enough levels to display.
    size_t requiredCount = (1 + m_currentPage) * m_pageSize;
    size_t hasCount = 0;

    for (auto id : m_allLevelIds) {
        if (m_levelCache.contains(id.levelId())) {
            if (this->isMatchingFilters(m_levelCache[id.levelId()])) {
                hasCount++;
            }
        }
    }

    log::debug("Continue loading, count = {} / {}", hasCount, requiredCount);

    if (hasCount >= requiredCount || !this->loadNextBatch()) {
        // either we already have enough levels, or there's just not enough at all.
        // simply halt.
        this->finishLoading();
    }
}

void LevelListLayer::finishLoading() {
    std::vector<Ref<GJGameLevel>> page;

    size_t counter = 0;
    size_t unloadedLevels = 0; // levels that haven't been loaded yet
    size_t reqMin = m_currentPage * m_pageSize;

    for (auto id : m_allLevelIds) {
        int lid = id.levelId();

        if (m_levelCache.contains(lid)) {
            if (this->isMatchingFilters(m_levelCache[lid])) {
                counter++;

                if (counter > reqMin && page.size() < m_pageSize) {
                    page.push_back(m_levelCache[lid]);
                }
            }
        } else if (!m_failedQueries.contains(lid)) {
            unloadedLevels++;
        }
    }

    log::debug("Finished loading, page size = {}, counter = {}, unloaded = {}, reqm = {}, failed = {}", page.size(), counter, unloadedLevels, reqMin, m_failedQueries.size());

    // TODO: idk if unloaded levels thing is rightt
    bool showNextPage = counter > (reqMin + m_pageSize) || unloadedLevels;

    // sort by player count descending
    std::sort(page.begin(), page.end(), [&](GJGameLevel* a, GJGameLevel* b) {
        auto aCount = this->findPlayerCountForLevel(a->m_levelID);
        auto bCount = this->findPlayerCountForLevel(b->m_levelID);

        if (!aCount) {
            return false;
        }

        if (!bCount) {
            return true;
        }

        return *aCount > *bCount;
    });

    m_list->clear();

    auto flevel = NetworkManagerImpl::get().getFeaturedLevel();

    for (auto level : page) {
        auto cell = static_cast<HookedLevelCell*>(new LevelCell("", 356.f, 90.f));
        cell->autorelease();
        cell->loadFromLevel(level);
        cell->setContentSize({356.f, 90.f});

        if (auto count = this->findPlayerCountForLevel(level->m_levelID)) {
            cell->updatePlayerCount(*count);
        }

        if (flevel && flevel->levelId == level->m_levelID) {
            globed::setFeatureTierForLevel(cell->m_level, flevel->rateTier);
            cell->setGlobedFeature(flevel->rateTier);
        }

        m_list->addCell(cell);
    }

    m_list->updateLayout();

    // show the buttons
    this->toggleLoadingUi(false);

    m_btnPagePrev->setVisible(m_currentPage > 0);
    m_btnPageNext->setVisible(showNextPage);

    m_loading = false;
}

bool LevelListLayer::loadNextBatch() {
    m_currentQuery.clear();

    for (auto id : m_allLevelIds) {
        int lid = id.levelId();

        if (m_levelCache.contains(lid) || m_failedQueries.contains(lid)) continue;

        m_currentQuery.push_back(lid);

        if (m_currentQuery.size() == 100) {
            break;
        }
    }

    if (m_currentQuery.empty()) {
        return false;
    }

    std::string query = fmt::format("{}", fmt::join(m_currentQuery, ","));

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;

    auto sobj = GJSearchObject::create(SearchType::Type26, query);
    glm->getOnlineLevels(sobj);

    return true;
}

std::optional<size_t> LevelListLayer::findPlayerCountForLevel(int levelId) {
    return asp::iter::from(m_playerCounts)
        .copied()
        .find([&](const auto& pair) { return pair.first.levelId() == levelId; })
        .transform([](const auto& pair) { return pair.second; });
}

bool LevelListLayer::isMatchingFilters(GJGameLevel* level) {
    using Difficulty = globed::Difficulty;
    using enum Difficulty;
    using enum Filters::RateTier;

    if (!level) return false;

    auto difficulty = globed::calcLevelDifficulty(level);

    // log::debug("Name = {}, diff = {}, demon diff = {}, is demon = {}, length = {}, epic = {}, featured = {}, stars = {}, coins = {}", level->m_levelName, (int) difficulty, (int)level->m_demonDifficulty, (int)level->m_demon, (int)level->m_levelLength, level->m_isEpic, level->m_featured, (int)level->m_stars, level->m_coins);
    // return true;

    // Difficulty
    if (!m_filters.difficulty.empty()) {
        auto difficulty2 = difficulty;

        // convert to HardDemon for this check
        if (difficulty == EasyDemon || difficulty == MediumDemon || difficulty == InsaneDemon || difficulty == ExtremeDemon) {
            difficulty2 = HardDemon;
        }

        // non demon filtering
        if (std::find(m_filters.difficulty.begin(), m_filters.difficulty.end(), difficulty2) == m_filters.difficulty.end()) {
            return false;
        }

        // demon filtering
        if (difficulty2 == HardDemon) {
            if (!level->m_demon) return false;

            if (!m_filters.demonDifficulty.empty()) {
                // check for specific demon difficulty
                if (std::find(m_filters.demonDifficulty.begin(), m_filters.demonDifficulty.end(), difficulty) == m_filters.demonDifficulty.end()) {
                    return false;
                }
            }
        }
    }

    if (!m_filters.length.empty()) {
        if (std::find(m_filters.length.begin(), m_filters.length.end(), level->m_levelLength) == m_filters.length.end()) {
            return false;
        }
    }

    if (m_filters.rateTier) {
        auto& tiers = m_filters.rateTier.value();

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

    if (m_filters.coins) {
        if (*m_filters.coins != (level->m_coins > 0)) {
            return false;
        }
    }

    if (m_filters.completed) {
        bool hasCompleted = level->m_dailyID > 0 ? level->m_orbCompletion > 99 : GameStatsManager::sharedState()->hasCompletedLevel(level);

        if (*m_filters.completed != hasCompleted) {
            return false;
        }
    }

    if (m_filters.twoPlayer && !level->m_twoPlayerMode) {
        return false;
    }

    if (m_filters.rated && level->m_stars <= 0) {
        return false;
    }

    // if (filters.song) {
    //     log::debug("Song id = {}, ids = {}", level->m_songID, level->m_songIDs);
    // }

    // if (m_filters.verifiedCoins && !level->m_coinsVerified) {
    //     return false;
    // }

    if (m_filters.original && level->m_originalLevel != 0) {
        return false;
    }

    return true;
}

void LevelListLayer::toggleLoadingUi(bool loading) {
    m_btnPagePrev->setVisible(!loading);
    m_btnPageNext->setVisible(!loading);
    m_btnRefresh->setVisible(!loading);
    m_btnFilters->setVisible(!loading);

    bool prevShown = m_circleShown;
    m_circleShown = loading;

    if (prevShown != loading) {
        if (loading) {
            m_loadingCircle->fadeIn();
        } else {
            m_loadingCircle->fadeOut();
        }
    }
}

void LevelListLayer::loadLevelsFinished(CCArray* arr, char const* q) {
    this->loadLevelsFinished(arr, q, -1);
}

void LevelListLayer::loadLevelsFinished(CCArray* arr, char const*, int) {
    for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(arr)) {
        m_levelCache[level->m_levelID] = level;
    }

    // check if there are any levels not present, add them to the failed list
    for (auto id : m_currentQuery) {
        if (!m_levelCache.contains(id)) {
            log::debug("Invalid level: {}", id);
            m_failedQueries.insert(id);
        }
    }

    this->continueLoading();
}

void LevelListLayer::loadLevelsFailed(char const* q) {
    this->loadLevelsFailed(q, -1);
}

void LevelListLayer::loadLevelsFailed(char const* q, int p) {
    log::warn("query failed ({}): {}", p, q);
    // ErrorQueues::get().warn(fmt::format("Failed to load levels: error {}", p));

    this->finishLoading();
}

LevelListLayer* LevelListLayer::create() {
    auto ret = new LevelListLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}

static std::vector<std::pair<globed::SessionId, uint16_t>> getFakeLevels() {
    std::initializer_list<std::pair<uint64_t, uint16_t>> levels = {
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

    return asp::iter::from(levels).copied().map([](auto pair) {
        return std::make_pair(globed::SessionId{pair.first}, pair.second);
    }).collect<std::vector<std::pair<globed::SessionId, uint16_t>>>();
}
