#pragma once

#include <util/singleton.hpp>
#include <vector>

struct DailyItem {
    int levelId;
    int edition;
    int rateTier;
};

class DailyManager : public SingletonBase<DailyManager> {
public:
    GJGameLevel* getStoredLevel();
    void setStoredLevel(GJGameLevel* level);
    void attachRatingSprite(int tier, cocos2d::CCNode* parent);

    void requestDailyItems();
    DailyItem getRecentDailyItem();
    GJSearchObject* getSearchObject();

private:
    GJGameLevel* storedLevel;
    std::vector<DailyItem> dailyLevelsList;
};