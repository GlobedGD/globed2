#pragma once
#include <Geode/Geode.hpp>
#include <data/types/gd.hpp>

class PlayerListPopup : public geode::Popup<> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;
    constexpr static float LIST_WIDTH = 338.f;
    constexpr static float LIST_HEIGHT = 200.f;

    ~PlayerListPopup();

    static PlayerListPopup* create();

protected:
    std::vector<PlayerPreviewAccountData> playerList;
    cocos2d::CCSequence* timeoutSequence = nullptr;
    LoadingCircle* loadingCircle = nullptr;
    GJCommentListLayer* listLayer = nullptr;

    bool setup() override;
    void onLoaded();
    void onLoadTimeout();
    void removeLoadingCircle();
};