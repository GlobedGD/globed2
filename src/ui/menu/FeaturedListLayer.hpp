#pragma once

#include <globed/prelude.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BaseLayer.hpp>

#include <cue/ListNode.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class FeaturedListLayer : public BaseLayer, public LevelManagerDelegate {
public:
    static FeaturedListLayer* create();
    ~FeaturedListLayer();

private:
    uint32_t m_page = 0;
    uint32_t m_totalPages = 0;
    CCLabelBMFont* m_levelsLabel;
    CCMenuItemSpriteExtra* m_prevButton;
    CCMenuItemSpriteExtra* m_nextButton;
    CCMenuItemSpriteExtra* m_refreshBtn;
    cue::ListNode* m_list;
    cue::LoadingCircle* m_circle = nullptr;
    bool m_loading = false;

    std::vector<std::vector<FeaturedLevelMeta>> m_pages;
    std::unordered_map<int, uint16_t> m_playerCounts;
    std::unordered_map<int, Ref<GJGameLevel>> m_levelCache;
    std::unordered_map<int, FeatureTier> m_levelToRateTier;
    std::unordered_set<int> m_failedQueries;
    std::vector<int> m_currentQuery;
    std::optional<MessageListener<msg::PlayerCountsMessage>> m_playerCountListener;
    std::optional<MessageListener<msg::FeaturedListMessage>> m_listListener;

    bool init() override;
    void update(float dt) override;
    void refreshLevels(bool force = false);

    void startLoading();
    void stopLoading();
    void toggleSideButtons();

    void populatePage(uint32_t page, const std::vector<FeaturedLevelMeta>& levels);
    void queryCurrentPage();
    void loadPageFromCache();

    void loadLevelsFinished(CCArray* levels, char const* key, int p2) override;
    void loadLevelsFailed(char const* key, int p1) override;
};

}
