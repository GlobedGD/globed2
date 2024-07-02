#pragma once

#include <vector>
#include <functional>

#include <util/singleton.hpp>
#include <managers/web.hpp>

struct GlobedFeaturedLevel {
    int id;
    int levelId;
    int rateTier;
};

struct GlobedFeaturedLevelList {
    std::vector<GlobedFeaturedLevel> levels;
};

class DailyManager : public SingletonBase<DailyManager> {
public:
    struct Page {
        std::vector<std::pair<GlobedFeaturedLevel, GJGameLevel*>> levels;
    };

    // Fetches the level or returns a cached copy
    void getStoredLevel(std::function<void(GJGameLevel*, const GlobedFeaturedLevel&)>&& callback, bool force = false);
    void resetStoredLevel();
    void clearWebCallback();

    // force clears all pages
    void getFeaturedLevels(int page, std::function<void(const Page&)>&& callback, bool force = false);

    void attachRatingSprite(int tier, cocos2d::CCNode* parent);

private:
    enum class FetchState {
        NotFetching,
        FetchingId,
        FetchingLevel,
    };

    WebRequestManager::Listener singleReqListener;
    std::function<void(GJGameLevel*, const GlobedFeaturedLevel&)> singleReqCallback;
    FetchState singleFetchState;

    GlobedFeaturedLevel storedLevelMeta;
    geode::Ref<GJGameLevel> storedLevel;


    WebRequestManager::Listener multipleReqListener;
    std::function<void(const Page&)> multipleReqCallback;
    FetchState multipleFetchState;
    int multipleFetchPage = 0;

    std::unordered_map<int, Page> storedMultiplePages;


    void onLevelMetaFetchedCallback(typename WebRequestManager::Event* e);
    void onFullLevelFetchedCallback(geode::Result<GJGameLevel*, int> e);

    void onMultipleMetaFetchedCallback(typename WebRequestManager::Event* e);
    void onMultipleFetchedCallback(geode::Result<cocos2d::CCArray*, int> e);
};