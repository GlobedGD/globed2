#pragma once

#include <defs/geode.hpp>

#include <managers/web.hpp>

class EditFeaturedLevelPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 200.f;
    static constexpr float POPUP_HEIGHT = 140.f;

    static EditFeaturedLevelPopup* create();

private:
    bool setup() override;
    void save();
    void createDiffButton();

    void onRequestComplete(typename WebRequestManager::Event* event);

    geode::TextInput* idInput;
    Ref<cocos2d::CCMenu> curDiffButton;
    int currIdx = 0;

    WebRequestManager::Listener reqListener;
};
