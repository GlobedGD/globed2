#pragma once
#include <defs/geode.hpp>

class GlobedCreditsPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 380.f;
    static constexpr float POPUP_HEIGHT = 250.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 190.f;

    static GlobedCreditsPopup* create();

protected:
    bool setup() override;

    cocos2d::CCArray* createCells();
};
