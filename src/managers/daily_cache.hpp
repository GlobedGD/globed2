#pragma once

#include <util/singleton.hpp>

class DailyCacheManager : public SingletonBase<DailyCacheManager> {
public:
    GJGameLevel* getStoredLevel();
    void setStoredLevel(GJGameLevel* level);
    static void attachRatingSprite(int tier, cocos2d::CCNode* parent);

private:
    GJGameLevel* storedLevel;
};