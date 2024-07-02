#pragma once

#include <defs/all.hpp>

class GlobedDailyLevelCell : public cocos2d::CCLayer {
public:
    static GlobedDailyLevelCell* create();
    ~GlobedDailyLevelCell();

    void reload();

protected:
    static constexpr float CELL_WIDTH = 380.f;
    static constexpr float CELL_HEIGHT = 116.f;

    bool init();
    bool onOpenLevel(cocos2d::CCObject*);

    Ref<cocos2d::CCMenu> menu;
    Ref<cocos2d::CCMenu> editionMenu;
    Ref<cocos2d::extension::CCScale9Sprite> darkBackground;
    Ref<cocos2d::extension::CCScale9Sprite> background;
    Ref<LoadingCircle> loadingCircle;
    CCMenuItemSpriteExtra *playButton = nullptr;
    GJGameLevel* level;

    short playerCount;
    int rating;
    int editionNum;

    void createCell(GJGameLevel* level);

    void downloadLevel(CCObject*);
};