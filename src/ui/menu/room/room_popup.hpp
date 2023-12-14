#pragma once
#include <defs.hpp>
#include <data/types/gd.hpp>

class RoomPopup : public geode::Popup<> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;
    constexpr static float LIST_WIDTH = 338.f;
    constexpr static float LIST_HEIGHT = 200.f;

    ~RoomPopup();

    static RoomPopup* create();

protected:
    std::vector<PlayerRoomPreviewAccountData> playerList;
    LoadingCircle* loadingCircle = nullptr;
    GJCommentListLayer* listLayer = nullptr;

    bool setup() override;
    void onLoaded();
    void removeLoadingCircle();
    void reloadPlayerList();
};