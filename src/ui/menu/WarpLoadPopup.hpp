#pragma once

#include <Geode/Bindings.hpp>
#include <ui/BasePopup.hpp>

namespace globed {

class WarpLoadPopup : public BasePopup<WarpLoadPopup, int, bool, bool>, public LevelDownloadDelegate {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    ~WarpLoadPopup();

private:
    cocos2d::CCLabelBMFont *m_statusLabel;
    int m_levelId;
    bool m_openLevel, m_replaceScene;
    bool m_finished = false;

    bool setup(int levelId, bool openLevel, bool replaceScene) override;
    void onClose(cocos2d::CCObject *) override;

    void onLoadedMeta(GJGameLevel *level);
    void onLoadedData(GJGameLevel *level);

    void levelDownloadFinished(GJGameLevel *p0) override;
    void levelDownloadFailed(int p0) override;
};

} // namespace globed