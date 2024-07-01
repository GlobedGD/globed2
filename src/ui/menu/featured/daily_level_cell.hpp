#pragma once

#include <defs/all.hpp>

class GlobedDailyLevelCell : public cocos2d::CCLayer {
public:
    static GlobedDailyLevelCell* create();
    ~GlobedDailyLevelCell();

protected:

    static constexpr float CELL_WIDTH = 380.f;
    static constexpr float CELL_HEIGHT = 116.f;

    bool init();
    bool onOpenLevel(cocos2d::CCObject*);

    cocos2d::CCMenu* menu;
    cocos2d::CCMenu* editionMenu;
    cocos2d::extension::CCScale9Sprite* darkBackground;
    cocos2d::extension::CCScale9Sprite* background;
    LoadingCircle* loadingCircle;
    CCMenuItemSpriteExtra *playButton = nullptr;
    GJGameLevel* level;

    short playerCount;
    int rating;
    int editionNum;

    void createCell(GJGameLevel* level);

    void downloadLevel(CCObject*);
};