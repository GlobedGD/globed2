#pragma once
#include <defs/geode.hpp>

#include "credits_cell.hpp"
#include <managers/web.hpp>
#include <ui/general/list/list.hpp>

class GlobedCreditsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 380.f;
    static constexpr float POPUP_HEIGHT = 250.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 190.f;

    static GlobedCreditsPopup* create();

protected:
    using CreditsList = GlobedListLayer<GlobedCreditsCell>;

    WebRequestManager::Listener eventListener;
    CreditsList* listLayer;

    void requestCallback(WebRequestManager::Task::Event* e);
    void setupFromCache();

    bool setup() override;

    cocos2d::CCArray* createCells();
};
