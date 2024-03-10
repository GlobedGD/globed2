#pragma once
#include <defs/all.hpp>

class GlobedUserActionsPopup : public geode::Popup<int> {
public:
    static constexpr float POPUP_WIDTH = 200.f;
    static constexpr float POPUP_HEIGHT = 90.f;

    static GlobedUserActionsPopup* create(int accountId);

    void remakeButtons();

private:
    Ref<cocos2d::CCMenu> buttonLayout;
    int accountId;

    bool setup(int accountId) override;
};
