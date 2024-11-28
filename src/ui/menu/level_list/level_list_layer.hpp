#pragma once

#include <defs/all.hpp>
#include <managers/settings.hpp>
#include <managers/daily_manager.hpp>
#include <data/types/misc.hpp>
#include <ui/general/loading_circle.hpp>
#include <util/gd.hpp>

class GlobedLevelListLayer : public cocos2d::CCLayer, LevelManagerDelegate {
public:
    static constexpr float LIST_WIDTH = 358.f;
    static constexpr float LIST_HEIGHT = 220.f;
    static constexpr size_t INCREASED_LIST_PAGE_SIZE = 100;
    static constexpr size_t LIST_PAGE_SIZE = 30;

    struct Filters {
        enum class RateTier {
            Unrated,
            Rate,
            Feature,
            Epic,
            Legendary,
            Mythic
        };

        std::set<util::gd::Difficulty> difficulty;
        std::set<util::gd::Difficulty> demonDifficulty;
        std::set<int> length;
        std::optional<std::set<RateTier>> rateTier;
        std::optional<bool> coins;
        std::optional<bool> completed;
        // std::optional<int> song;
        bool twoPlayer = false;
        bool rated = false;
        // bool verifiedCoins = false;
        bool original = false;

        bool operator==(const Filters&) const = default;
    };

    static GlobedLevelListLayer* create();
    ~GlobedLevelListLayer();

private:
    GJListLayer* listLayer = nullptr;
    Ref<CCMenuItemSpriteExtra> btnPagePrev, btnPageNext, btnRefresh, btnFilters;
    Ref<BetterLoadingCircle> loadingCircle;
    GlobedFeaturedLevel currentFeaturedLevel;

    std::unordered_map<LevelId, uint32_t> playerCounts;
    std::unordered_map<LevelId, Ref<GJGameLevel>> levelCache;

    std::vector<LevelId> allLevelIds;
    std::set<LevelId> failedQueries;
    std::vector<LevelId> currentQuery;

    size_t pageSize = 0;
    int currentPage = 0;
    bool loading = false;

    Filters filters;

    bool init() override;
    void keyBackClicked() override;

    void onOpenFilters();
    void onRefresh();
    void onPrevPage();
    void onNextPage();

    void startLoadingForPage();
    void continueLoading();
    void finishLoading();
    void showLoadingUi();
    // Load next (n <= 100) levels, returns false if we already loaded all levels
    bool loadNextBatch();

    bool isMatchingFilters(GJGameLevel* level);

    void loadLevelsFinished(cocos2d::CCArray*, char const*) override;
    void loadLevelsFinished(cocos2d::CCArray*, char const*, int) override;
    void loadLevelsFailed(char const*) override;
    void loadLevelsFailed(char const*, int) override;
};