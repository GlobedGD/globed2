#pragma once

#include <ui/BasePopup.hpp>
#include <Geode/Bindings.hpp>

namespace globed {

class WarpLoadPopup : public BasePopup, public LevelDownloadDelegate {
public:
    static WarpLoadPopup* create(int levelId, bool openLevel, bool replaceScene);
    ~WarpLoadPopup();

private:
    cocos2d::CCLabelBMFont* m_statusLabel;
    int m_levelId;
    bool m_openLevel, m_replaceScene;
    bool m_finished = false;

    bool init(int levelId, bool openLevel, bool replaceScene);
    void onClose(cocos2d::CCObject*) override;

    void onLoadedMeta(GJGameLevel* level);
    void onLoadedData(GJGameLevel* level);

    void levelDownloadFinished(GJGameLevel* p0) override;
    void levelDownloadFailed(int p0) override;
};

}