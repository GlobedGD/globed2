#pragma once

#include <ui/BasePopup.hpp>
#include <Geode/Bindings.hpp>

namespace globed {

class WarpLoadPopup : public BasePopup<WarpLoadPopup, int, bool>, public LevelDownloadDelegate, public LevelManagerDelegate {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    ~WarpLoadPopup();

private:
    cocos2d::CCLabelBMFont* m_statusLabel;
    int m_levelId;
    bool m_openLevel;

    bool setup(int levelId, bool openLevel) override;
    void onClose(cocos2d::CCObject*) override;

    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1, int p2) override;
    void loadLevelsFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadLevelsFailed(char const* p0, int p1) override;
    void loadLevelsFailed(char const* p0) override;

    void levelDownloadFinished(GJGameLevel* p0) override;
    void levelDownloadFailed(int p0) override;
};

}