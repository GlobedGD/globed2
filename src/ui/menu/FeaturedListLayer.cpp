#include "FeaturedListLayer.hpp"
#include <globed/core/RoomManager.hpp>
#include <globed/util/gd.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/LevelCell.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

static constexpr CCSize LIST_SIZE {358.f, 220.f};

namespace globed {

bool FeaturedListLayer::init() {
    if (!BaseLayer::init()) return false;

    geode::addSideArt(this, SideArt::Bottom);

    auto winSize = CCDirector::get()->getWinSize();

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown))
        .zOrder(2)
        .parent(this)
        .pos(winSize / 2.f)
        .id("level-list"_spr);

    Build<CCSprite>::create("icon-featured-label.png"_spr)
        .zOrder(10)
        .pos(LIST_SIZE.width / 2.f, LIST_SIZE.height / 2.f + 133.f) // TODO
        .parent(m_list);

    // refresh button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->refreshLevels(true);
        })
        .id("btn-refresh")
        .pos(winSize.width - 35.f, 35.f)
        .store(m_refreshBtn)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .zOrder(2)
        .parent(this);

    // levels label
    m_levelsLabel = Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .id("level-count-label")
        .pos(winSize.width - 7, winSize.height - 2)
        .scale(0.45f)
        .zOrder(2)
        .anchorPoint({1, 1})
        .parent(this);

    constexpr float pageBtnPadding = 20.f;

    // pages buttons
    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .intoMenuItem([this](auto) {
            m_page--;
            this->refreshLevels(false);
        })
        .id("btn-prev-page")
        .pos(pageBtnPadding, winSize.height / 2)
        .store(m_prevButton)
        .intoNewParent(CCMenu::create())
        .id("prev-page-menu")
        .pos(0.f, 0.f)
        .parent(this);

    CCSprite* btnSprite;
    Build<CCSprite>::createSpriteName("GJ_arrow_03_001.png")
        .store(btnSprite)
        .intoMenuItem([this](auto) {
            m_page++;
            this->refreshLevels(false);
        })
        .id("btn-next-page")
        .pos(winSize.width - pageBtnPadding, winSize.height / 2)
        .store(m_nextButton)
        .intoNewParent(CCMenu::create())
        .id("next-page-menu")
        .pos(0.f, 0.f)
        .parent(this);

    btnSprite->setFlipX(true);

    auto& nm = NetworkManagerImpl::get();
    m_playerCountListener = nm.listen<msg::PlayerCountsMessage>([this](const auto& packet) {
        m_playerCounts.clear();

        for (auto& [k, v] : packet.counts) {
            m_playerCounts[k.levelId()] = v;
        }

        return ListenerResult::Continue;
    });

    m_listListener = nm.listen<msg::FeaturedListMessage>([this](const auto& packet) {
        log::debug("Received featured list page {}/{} ({} levels)", packet.page + 1, packet.totalPages, packet.levels.size());

        m_totalPages = packet.totalPages;
        m_pages.resize(m_totalPages);
        this->populatePage(packet.page, packet.levels);
        this->queryCurrentPage();
        return ListenerResult::Continue;
    });

    this->refreshLevels();
    this->scheduleUpdate();

    return true;
}

FeaturedListLayer::~FeaturedListLayer() {
    GameLevelManager::get()->m_levelManagerDelegate = nullptr;
}

void FeaturedListLayer::populatePage(uint32_t page, const std::vector<FeaturedLevelMeta>& levels) {
    m_pages[page] = levels;
    for (auto& level : levels) {
        m_levelToRateTier[level.levelId] = level.rateTier;
    }
}

void FeaturedListLayer::queryCurrentPage() {
    std::string query;
    m_currentQuery.clear();

    for (auto& level : m_pages.at(m_page)) {
        if (m_failedQueries.contains(level.levelId) || m_levelCache.contains(level.levelId)) {
            continue;
        }

        query += fmt::format("{},", level.levelId);
        m_currentQuery.push_back(level.levelId);
    }

    if (query.empty()) {
        // no levels to fetch, everything cached or failed
        this->loadPageFromCache();
        return;
    }

    log::debug("Querying featured levels: {}", query);

    query.pop_back();

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(GJSearchObject::create(SearchType::Type19, query));
}

void FeaturedListLayer::loadPageFromCache() {
    this->stopLoading();

    std::vector<uint64_t> sessions;

    for (auto& level : m_pages.at(m_page)) {
        auto it = m_levelCache.find(level.levelId);

        if (it == m_levelCache.end()) {
            continue;
        }

        auto lvl = it->second;
        globed::reorderDownloadedLevel(lvl);

        auto cell = new LevelCell("", 356.f, 90.f);
        cell->autorelease();
        cell->loadFromLevel(lvl);
        cell->setContentSize({356.f, 90.f});
        static_cast<HookedLevelCell*>(cell)->setGlobedFeature(m_levelToRateTier[level.levelId]);

        m_list->addCell(cell);

        sessions.push_back(RoomManager::get().makeSessionId(lvl->m_levelID));
    }

    auto& nm = NetworkManagerImpl::get();

    int highest = nm.getFeaturedLevel().value_or({}).edition;
    int pageMin = m_page * 10 + 1;
    int pageMax = std::min<int>((m_page + 1) * 10, highest);

    m_levelsLabel->setString(fmt::format("{} to {} of {}", pageMin, pageMax, highest).c_str());

    // request player counts
    nm.sendRequestPlayerCounts(sessions);
}

void FeaturedListLayer::toggleSideButtons() {
    m_prevButton->setVisible(m_page > 0);
    m_nextButton->setVisible(m_page + 1 < m_totalPages);
    m_refreshBtn->setVisible(true);
}

void FeaturedListLayer::refreshLevels(bool force) {
    this->startLoading();

    // retrieve the current page
    NetworkManagerImpl::get().sendGetFeaturedList(m_page);
}

void FeaturedListLayer::update(float dt) {
    for (auto cell : m_list->iter<HookedLevelCell>()) {
        auto it = m_playerCounts.find(cell->m_level->m_levelID);

        if (it != m_playerCounts.end()) {
            cell->updatePlayerCount(it->second, true);
        } else {
            cell->updatePlayerCount(0, true);
        }
    }
}

void FeaturedListLayer::startLoading() {
    m_loading = true;
    m_prevButton->setVisible(false);
    m_nextButton->setVisible(false);
    m_refreshBtn->setVisible(false);

    cue::resetNode(m_circle);
    m_circle = cue::LoadingCircle::create(true);
    m_circle->addToLayer(m_list);

    m_list->clear();
}

void FeaturedListLayer::stopLoading() {
    m_loading = false;
    m_circle->fadeOut();
    this->toggleSideButtons();
}

void FeaturedListLayer::loadLevelsFinished(CCArray* levels, char const* key, int p2) {
    for (GJGameLevel* level : CCArrayExt<GJGameLevel*>(levels)) {
        m_levelCache[level->m_levelID] = level;
    }

    // check if there are any levels not present, add them to the failed list
    for (auto id : m_currentQuery) {
        if (!m_levelCache.contains(id)) {
            log::warn("Featured level wasn't returned in query: {}", id);
            m_failedQueries.insert(id);
        }
    }

    this->loadPageFromCache();
}

void FeaturedListLayer::loadLevelsFailed(char const* key, int p1) {
    log::warn("Failed to load featured levels: {} (key: {})", p1, key);
    this->stopLoading();
}

FeaturedListLayer* FeaturedListLayer::create() {
    auto ret = new FeaturedListLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
