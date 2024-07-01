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

class DailyManager : public SingletonBase<DailyManager> {
public:
    // Fetches the level or returns a cached copy
    void getStoredLevel(std::function<void(GJGameLevel*, const GlobedFeaturedLevel&)>&& callback, bool force = false);
    void clearWebCallback();

    // force clears all pages
    void getFeaturedLevels(int page, std::function<void(std::vector<std::pair<GJGameLevel*, GlobedFeaturedLevel>>)>&& callback, bool force = false);

    void attachRatingSprite(int tier, cocos2d::CCNode* parent);

private:
    enum class FetchState {
        NotFetching,
        FetchingId,
        FetchingLevel,
        FetchingFullLevel,
        Complete,
    };

    WebRequestManager::Listener singleReqListener;
    std::function<void(GJGameLevel*, const GlobedFeaturedLevel&)> singleReqCallback;
    FetchState singleFetchState;

    WebRequestManager::Listener multipleReqListener;
    std::function<void(std::vector<std::pair<GJGameLevel*, GlobedFeaturedLevel>>)> multipleReqCallback;
    FetchState multipleFetchState;

    GlobedFeaturedLevel storedLevelMeta;
    geode::Ref<GJGameLevel> storedLevel;

    void onLevelMetaFetchedCallback(typename WebRequestManager::Event* e);
    void onLevelFetchedCallback(geode::Result<GJGameLevel*, int> e);
    void onFullLevelFetchedCallback(geode::Result<GJGameLevel*, int> e);

    void onMultipleMetaFetchedCallback(typename WebRequestManager::Event* e);
};