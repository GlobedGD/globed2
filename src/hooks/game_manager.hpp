#pragma once
#include <defs/geode.hpp>

#include <Geode/modify/GameManager.hpp>

class $modify(HookedGameManager, GameManager) {
    std::unordered_map<int, std::unordered_map<int, Ref<cocos2d::CCTexture2D>>> iconCache;
    std::unordered_set<std::string> loadedFrames;
    int lastSceneEnum;
    bool assetsPreloaded = false;
    bool deathEffectsPreloaded = false;

    static void onModify(auto& self) {
        (void) self.setHookPriority("GameManager::returnToLastScene", -999999999);
    }

    $override
    void returnToLastScene(GJGameLevel* level);

    $override
    cocos2d::CCTexture2D* loadIcon(int iconId, int iconType, int iconRequestId);

    $override
    void unloadIcon(int iconId, int iconType, int idk);

    $override
    void loadDeathEffect(int de);

    $override
    void reloadAllStep2();

    // Load multiple icons in parallel. Range is inclusive [startId; endId]
    void loadIconsBatched(int iconType, int startId, int endId);

    struct BatchedIconRange {
        int iconType;
        int startId;
        int endId;
    };

    void loadIconsBatched(const std::vector<BatchedIconRange>& ranges);

    bool getAssetsPreloaded();
    void setAssetsPreloaded(bool state);

    bool getDeathEffectsPreloaded();
    void setDeathEffectsPreloaded(bool state);

    void resetAssetPreloadState();

    cocos2d::CCTexture2D* getCachedIcon(int iconId, int iconType);

    void setLastSceneEnum(int n = -1);
};
