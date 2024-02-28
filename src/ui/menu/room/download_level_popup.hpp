#pragma once
#include <defs/all.hpp>

class DownloadLevelPopup : public geode::Popup<int>, public LevelDownloadDelegate, public LevelManagerDelegate {
public:
    static constexpr float POPUP_WIDTH = 180.f;
    static constexpr float POPUP_HEIGHT = 80.f;

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override;
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadLevelsFailed(char const* p0, int p1) override;
    void loadLevelsFailed(char const* p0) override;
    void setupPageInfo(gd::string p0, char const* p1) override;

    void levelDownloadFinished(GJGameLevel* p0) override;
    void levelDownloadFailed(int p0) override;

    static DownloadLevelPopup* create(int levelId);
    void onClose(cocos2d::CCObject*) override;

private:
    cocos2d::CCLabelBMFont* statusLabel;

    bool setup(int levelId) override;
};
