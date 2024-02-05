#pragma once
#include <defs.hpp>

class GlobedUserListPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 200.f;

    static GlobedUserListPopup* create();

private:
    GJCommentListLayer* listLayer = nullptr;

    bool setup() override;
    void reloadList(float);
    void hardRefresh();
    cocos2d::CCArray* createPlayerCells();
};
