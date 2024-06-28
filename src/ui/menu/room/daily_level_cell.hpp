#pragma once
#include <defs/all.hpp>

class GlobedDailyLevelCell : public cocos2d::CCLayer, public LevelDownloadDelegate, public LevelManagerDelegate {
public:
    static GlobedDailyLevelCell* create(int levelId, int edition, int rateTier);

protected:

    static constexpr float CELL_WIDTH = 380.f;
    static constexpr float CELL_HEIGHT = 116.f;

    bool init(int levelId, int edition, int rateTier);
    bool onOpenLevel(cocos2d::CCObject*);

    cocos2d::CCMenu* menu;
    cocos2d::extension::CCScale9Sprite* darkBackground;
    cocos2d::extension::CCScale9Sprite* background;
    LoadingCircle* loadingCircle;
    CCMenuItemSpriteExtra *playButton = nullptr;

    GJGameLevel* level = nullptr;

    void downloadLevel(CCObject*);
    virtual void levelDownloadFinished(GJGameLevel*) override;
    virtual void levelDownloadFailed(int) override;

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override;
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadLevelsFailed(char const* p0, int p1) override;
    void loadLevelsFailed(char const* p0) override;
};