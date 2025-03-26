#pragma once

#include <defs/geode.hpp>
#include <ui/general/progressbar.hpp>

class SwagConnectionTestPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 340.f;
    static constexpr float POPUP_HEIGHT = 180.f;

    static SwagConnectionTestPopup* create();

    bool setup() override;
    void onClose(CCObject*) override;

    void setProgress(double p);
    void startTesting();
    void animateIn();

private:
    // xbox style
    bool xboxStyle = false;
    std::vector<cocos2d::CCLabelBMFont*> dotSprites;
    cocos2d::CCSprite* consoleIcon = nullptr;
    cocos2d::CCLabelBMFont* consoleLabel = nullptr;
    cocos2d::CCSprite* networkIcon = nullptr;
    cocos2d::CCLabelBMFont* networkLabel = nullptr;
    cocos2d::CCSprite* internetIcon = nullptr;
    cocos2d::CCLabelBMFont* internetLabel = nullptr;
    cocos2d::CCSprite* xblIcon = nullptr;
    cocos2d::CCLabelBMFont* xblLabel = nullptr;
    Ref<cocos2d::CCSprite> crossIcon = nullptr;
    size_t xbCurState = 0;

    class StatusCell;

    // common ui
    StatusCell *netCell, *internetCell, *srvCell;

    bool reallyClose = false;

    void createCommonUi();
    void createXboxStyleUi();

    void xbPutCrossAt(size_t idx);
    void xbStopPulseDots();
    void xbSetDotsForState();
    void xbPulseDots();
    void xbRedoDots();
    void xbFail();
    void xbStartNetwork();
    void xbStartInternet();
    void xbStartXbl();
    void xbSuccess();
};
