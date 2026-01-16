#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/util/gd.hpp>
#include <ui/BaseLayer.hpp>

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class LevelListLayer : public BaseLayer, LevelManagerDelegate {
public:
    static LevelListLayer *create();
    ~LevelListLayer();

    struct Filters {
        enum class RateTier { Unrated, Rate, Feature, Epic, Legendary, Mythic };

        std::set<Difficulty> difficulty;
        std::set<Difficulty> demonDifficulty;
        std::set<int> length;
        std::optional<std::set<RateTier>> rateTier;
        std::optional<bool> coins;
        std::optional<bool> completed;
        // std::optional<int> song;
        bool twoPlayer = false;
        bool rated = false;
        // bool verifiedCoins = false;
        bool original = false;

        bool operator==(const Filters &) const = default;
    };

private:
    friend class LevelFiltersPopup;

    cue::ListNode *m_list = nullptr;
    geode::Ref<CCMenuItemSpriteExtra> m_btnPagePrev, m_btnPageNext, m_btnRefresh, m_btnFilters;
    geode::Ref<cue::LoadingCircle> m_loadingCircle;
    // GlobedFeaturedLevel currentFeaturedLevel;

    std::optional<MessageListener<msg::LevelListMessage>> m_levelListener;
    std::unordered_map<SessionId, uint32_t> m_playerCounts;
    std::unordered_map<int, geode::Ref<GJGameLevel>> m_levelCache;

    std::vector<SessionId> m_allLevelIds;
    std::set<int> m_failedQueries;
    std::vector<int> m_currentQuery;

    size_t m_pageSize = 0;
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_circleShown = false;

    Filters m_filters;

    bool init() override;

    void onLoaded(const std::vector<std::pair<SessionId, uint16_t>> &levels);

    void onOpenFilters();
    void onRefresh();
    void onPrevPage();
    void onNextPage();

    void startLoadingForPage();
    void continueLoading();
    void finishLoading();
    void toggleLoadingUi(bool loading);
    // Load next (n <= 100) levels, returns false if we already loaded all levels
    bool loadNextBatch();

    std::optional<size_t> findPlayerCountForLevel(int levelId);

    bool isMatchingFilters(GJGameLevel *level);

    void loadLevelsFinished(cocos2d::CCArray *, char const *) override;
    void loadLevelsFinished(cocos2d::CCArray *, char const *, int) override;
    void loadLevelsFailed(char const *) override;
    void loadLevelsFailed(char const *, int) override;
};

} // namespace globed
