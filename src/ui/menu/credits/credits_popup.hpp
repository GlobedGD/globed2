#pragma once
#include <defs/geode.hpp>

#include <managers/web.hpp>

class GlobedCreditsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 380.f;
    static constexpr float POPUP_HEIGHT = 250.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 190.f;

    static GlobedCreditsPopup* create();

protected:
    WebRequestManager::Listener eventListener;
    geode::ScrollLayer* scrollLayer;

    void requestCallback(WebRequestManager::Task::Event* e);
    void setupFromCache();

    bool setup() override;

    cocos2d::CCArray* createCells();
};
