#include "daily_manager.hpp"

#include <algorithm>

#include <managers/error_queues.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

class DummyLevelFetchNode : public CCObject, public LevelManagerDelegate, public LevelDownloadDelegate {
public:
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override {
        if (this->multipleFetchCallback) {
            this->multipleFetchCallback(Ok(p0));
        }
    }

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override {
        this->loadLevelsFinished(p0, p1, 0);
    }

    void loadLevelsFailed(char const* p0, int p1) override {
        if (this->multipleFetchCallback) {
            this->multipleFetchCallback(Err(p1));
        }
    }

    void loadLevelsFailed(char const* p0) override {
        this->loadLevelsFailed(p0, 0);
    }

    std::function<void(Result<CCArray*, int>)> multipleFetchCallback;

    void levelDownloadFinished(GJGameLevel* level) override {
        if (this->downloadCallback) {
            this->downloadCallback(Ok(level));
        }
    }

    void levelDownloadFailed(int x) override {
        if (this->downloadCallback) {
            this->downloadCallback(Err(x));
        }
    }

    std::function<void(Result<GJGameLevel*, int>)> downloadCallback;
};

template <>
struct matjson::Serialize<GlobedFeaturedLevel> {
    static GlobedFeaturedLevel from_json(const matjson::Value& value) {
        return GlobedFeaturedLevel {
            .id = value["id"].as_int(),
            .levelId = value["level_id"].as_int(),
            .rateTier = value["rate_tier"].as_int(),
        };
    }

    static matjson::Value to_json(const GlobedFeaturedLevel& user) {
        throw std::runtime_error("unimplemented");
    }

    static bool is_json(const matjson::Value& value) {
        if (
            !value.contains("id")
            || !value.contains("level_id")
            || !value.contains("rate_tier")
        ) {
            return false;
        }

        return value["id"].is_number() && value["level_id"].is_number() && value["rate_tier"].is_number();
    }
};

template <>
struct matjson::Serialize<GlobedFeaturedLevelList> {
    static GlobedFeaturedLevelList from_json(const matjson::Value& value) {
        std::vector<GlobedFeaturedLevel> levels;

        for (auto& l : value.as_array()) {
            levels.push_back(l.as<GlobedFeaturedLevel>());
        }

        return GlobedFeaturedLevelList {
            .levels = levels
        };
    }

    static matjson::Value to_json(const GlobedFeaturedLevelList& user) {
        throw std::runtime_error("unimplemented");
    }

    static bool is_json(const matjson::Value& value) {
        if (!value.is_array()) return false;

        for (auto& val : value.as_array()) {
            if (!val.is<GlobedFeaturedLevel>()) return false;
        }

        return true;
    }
};

static DummyLevelFetchNode* fetchNode = new DummyLevelFetchNode();

void DailyManager::getStoredLevel(std::function<void(GJGameLevel*, const GlobedFeaturedLevel&)>&& callback, bool force) {
    if (storedLevel != nullptr && !force) {
        callback(storedLevel, storedLevelMeta);
        return;
    }

    this->singleReqCallback = std::move(callback);

    if (singleFetchState != FetchState::NotFetching) {
        return;
    }

    singleFetchState = FetchState::FetchingId;
    storedLevel = nullptr;
    storedLevelMeta = {};

    // fetch level id from current central server
    auto req = WebRequestManager::get().fetchFeaturedLevel();
    singleReqListener.bind(this, &DailyManager::onLevelMetaFetchedCallback);
    singleReqListener.setFilter(std::move(req));
}

void DailyManager::resetStoredLevel() {
    if (singleFetchState != FetchState::NotFetching) {
        return;
    }

    storedLevelMeta = {};
    storedLevel = nullptr;
    singleFetchState = FetchState::NotFetching;
}

void DailyManager::onLevelMetaFetchedCallback(typename WebRequestManager::Event* e) {
    if (!e || !e->getValue()) return;

    auto result = std::move(*e->getValue());
    if (!result) {
        auto err = result.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to fetch the current featured level.\n\nReason: <cy>{}</c>", util::format::webError(err)));
        singleFetchState = FetchState::NotFetching;
        return;
    }

    auto val = result.unwrap();

    std::string parseError;
    auto parsed_ = matjson::parse(val, parseError);


    if (!parsed_ || !parsed_->is<GlobedFeaturedLevel>()) {
        if (parsed_) {
            log::warn("failed to parse featured level:\n{}", parsed_.value().dump());
        }

        ErrorQueues::get().error(fmt::format("Failed to fetch the current featured level.\n\nReason: <cy>parsing failed: {}</c>", parsed_.has_value() ? "invalid json structure" : parseError));
        singleFetchState = FetchState::NotFetching;
        return;
    }

    storedLevelMeta = std::move(parsed_.value()).as<GlobedFeaturedLevel>();

    // fetch level from gd servers

    auto* glm = GameLevelManager::get();

    glm->m_levelDownloadDelegate = fetchNode;
    fetchNode->downloadCallback = [this](Result<GJGameLevel*, int> level) {
        this->onFullLevelFetchedCallback(level);
    };

    glm->downloadLevel(storedLevelMeta.levelId, false);

    singleFetchState = FetchState::FetchingLevel;
}

void DailyManager::onFullLevelFetchedCallback(Result<GJGameLevel*, int> level) {
    auto* glm = GameLevelManager::get();
    glm->m_levelDownloadDelegate = nullptr;

    if (level.isOk()) {
        storedLevel = level.unwrap();

        singleFetchState = FetchState::NotFetching;

        if (this->singleReqCallback) {
            this->singleReqCallback(storedLevel, storedLevelMeta);
        }
    } else {
        singleFetchState = FetchState::NotFetching;

        int err = level.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to download featured level from the server: <cy>code {}</c>", err));
    }

}

void DailyManager::clearWebCallback() {
    singleReqCallback = {};
}

void DailyManager::getFeaturedLevels(int page, std::function<void(const Page&)>&& callback, bool force) {
    if (multipleFetchState == FetchState::NotFetching) {
        if (storedMultiplePages.contains(page)) {
            callback(storedMultiplePages[page]);
            return;
        }
    } else if (multipleFetchPage == page) {
        this->multipleReqCallback = std::move(callback);
        return;
    }

    this->multipleReqCallback = std::move(callback);
    this->multipleReqListener.getFilter().cancel();
    this->multipleFetchPage = page;

    multipleFetchState = FetchState::FetchingId;

    // fetch level ids from the central server
    auto req = WebRequestManager::get().fetchFeaturedLevelHistory(page);
    multipleReqListener.bind(this, &DailyManager::onMultipleMetaFetchedCallback);
    multipleReqListener.setFilter(std::move(req));
}

void DailyManager::onMultipleMetaFetchedCallback(typename WebRequestManager::Event* e) {
    if (!e || !e->getValue()) return;

    auto result = std::move(*e->getValue());

    if (!result) {
        auto err = result.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to fetch the current featured level.\n\nReason: <cy>{}</c>", util::format::webError(err)));
        multipleFetchState = FetchState::NotFetching;
        return;
    }

    auto val = result.unwrap();

    std::string parseError;
    auto parsed_ = matjson::parse(val, parseError);

    if (!parsed_ || !parsed_->is<GlobedFeaturedLevelList>()) {
        if (parsed_) {
            log::warn("failed to parse featured level list:\n{}", parsed_.value().dump());
        }

        ErrorQueues::get().error(fmt::format("Failed to fetch the featured level history.\n\nReason: <cy>parsing failed: {}</c>", parsed_.has_value() ? "invalid json structure" : parseError));
        multipleFetchState = FetchState::NotFetching;
        return;
    }

    auto levelList = std::move(parsed_.value()).as<GlobedFeaturedLevelList>();

    // sort
    std::sort(levelList.levels.begin(), levelList.levels.end(), [](auto& level1, auto& level2) {
        return level1.id > level2.id;
    });

    // fetch level from gd servers

    fetchNode->multipleFetchCallback = [this](Result<cocos2d::CCArray*, int> levels) {
        this->onMultipleFetchedCallback(levels);
    };

    multipleFetchState = FetchState::FetchingLevel;
    auto* glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = fetchNode;

    // join to a string
    std::string levelIds;
    bool first = true;
    for (auto& level : levelList.levels) {
        if (!first) levelIds += ",";
        first = false;

        levelIds += std::to_string(level.levelId);
    }

    glm->getOnlineLevels(GJSearchObject::create(SearchType::Type19, levelIds));

    // set stuff
    storedMultiplePages[multipleFetchPage] = Page {};

    for (auto& level : levelList.levels) {
        storedMultiplePages[multipleFetchPage].levels.push_back(std::make_pair(level, nullptr));
    }
}

void DailyManager::onMultipleFetchedCallback(Result<cocos2d::CCArray*, int> e) {
    if (e.isOk()) {
        for (auto level : CCArrayExt<GJGameLevel*>(e.unwrap())) {
            auto& levels = storedMultiplePages[multipleFetchPage].levels;

            for (auto& l : levels) {
                if (l.first.levelId == level->m_levelID) {
                    l.second = level;
                    break;
                }
            }
        }

        multipleFetchState = FetchState::NotFetching;

        if (this->multipleReqCallback) {
            this->multipleReqCallback(storedMultiplePages[multipleFetchPage]);
        }
    } else {
        multipleFetchState = FetchState::NotFetching;

        int err = e.unwrapErr();
        ErrorQueues::get().error(fmt::format("Failed to download featured levels from the server: <cy>code {}</c>", err));
    }

}

// void DailyManager::requestDailyItems() {
//     dailyLevelsList.clear();

//     dailyLevelsList.push_back({102837084, 1, 2});
//     dailyLevelsList.push_back({107224663, 2, 2});
//     dailyLevelsList.push_back({105117679, 3, 0});
//     dailyLevelsList.push_back({100643019, 4, 1});
// }

// int DailyManager::getRatingFromID(int levelId) {
//     int rating = -1;
//     // for (DailyItem item : dailyLevelsList) {
//     //     if (item.levelId == levelId) {
//     //         rating = item.rateTier;
//     //         break;
//     //     }
//     // }
//     return rating;
// }

// std::vector<long long> DailyManager::getLevelIDs() {
//     std::vector<long long> vec;
//     // for (DailyItem item : dailyLevelsList) {
//     //     //log::info("test {}", item.levelId);
//     //     vec.push_back(item.levelId);
//     // }
//     return vec;
// }

// std::vector<long long> DailyManager::getLevelIDsReverse() {
//     std::vector<long long> vec;
//     // for (DailyItem item : dailyLevelsList) {
//     //     //log::info("test {}", item.levelId);
//     //     vec.push_back(item.levelId);
//     // }
//     std::reverse(vec.begin(), vec.end());
//     return vec;
// }

// GlobedFeaturedLevel DailyManager::getRecentDailyItem() {
//     return dailyLevelsList.back();
// }

void DailyManager::attachRatingSprite(int tier, CCNode* parent) {
    if (!parent) return;

    if (parent->getChildByID("globed-rating"_spr)) {
        return;
    }

    for (CCNode* child : CCArrayExt<CCNode*>(parent->getChildren())) {
        child->setVisible(false);
    }

    CCSprite* spr;
    switch (tier) {
        case 1:
            spr = CCSprite::createWithSpriteFrameName("icon-epic.png"_spr);
            break;
        case 2:
            spr = CCSprite::createWithSpriteFrameName("icon-outstanding.png"_spr);
            break;
        case 0:
        default:
            spr = CCSprite::createWithSpriteFrameName("icon-featured.png"_spr);
            break;
    }

    spr->setZOrder(-1);
    spr->setPosition(parent->getScaledContentSize() / 2);
    spr->setID("globed-rating"_spr);
    parent->addChild(spr);

    if (tier == 2) {
        CCSprite* overlay = Build<CCSprite>::createSpriteName("icon-outstanding-overlay.png"_spr)
            .pos(parent->getScaledContentSize() / 2 + CCPoint{0.f, 2.f})
            .blendFunc({GL_ONE, GL_ONE})
            .color({200, 255, 255})
            .opacity(175)
            .zOrder(1)
            .parent(parent);

        auto particle = GameToolbox::particleFromString("26a-1a1.25a0.3a16a90a62a4a0a20a20a0a16a0a0a0a0a4a2a0a0a0.341176a0a1a0a0.635294a0a1a0a0a1a0a0a0.247059a0a1a0a0.498039a0a1a0a0.16a0a0.23a0a0a0a0a0a0a0a0a2a1a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0a0", nullptr, false);
        particle->setPosition(parent->getScaledContentSize() / 2 + CCPoint{0.f, 4.f});
        particle->setZOrder(-2);
        parent->addChild(particle);
    }
}

// GJSearchObject* DailyManager::getSearchObject() {
//     std::stringstream download;
//     bool first = true;

//     for (int i = dailyLevelsList.size() - 1; i >= 0; i--) {
//         if (!first) {
//             download << ",";
//         }

//         download << dailyLevelsList.at(i).levelId;
//         first = false;
//     }

//     GJSearchObject* searchObj = GJSearchObject::create(SearchType::Type19, download.str());
//     return searchObj;
// }
