#pragma once
#include <defs/all.hpp>

class GlobedUserListPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 190.f;

    static GlobedUserListPopup* create();

private:
    GJCommentListLayer* listLayer = nullptr;
    bool volumeSortEnabled = false;
    Slider* volumeSlider = nullptr;

    bool setup() override;
    void reloadList(float);
    void reorderWithVolume(float);
    void hardRefresh();
    cocos2d::CCArray* createPlayerCells();
    void onToggleVoiceSort(cocos2d::CCObject* sender);
    void onVolumeChanged(cocos2d::CCObject* sender);
};
