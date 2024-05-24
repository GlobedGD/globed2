#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

class RoomPopup : public geode::Popup<> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;
    constexpr static float LIST_WIDTH = 340.f;
    constexpr static float LIST_HEIGHT = 180.f;

    static RoomPopup* create();

    void reloadPlayerList(bool sendPacket = true);

protected:
    std::vector<PlayerRoomPreviewAccountData> playerList;
    std::vector<PlayerRoomPreviewAccountData> filteredPlayerList;

    LoadingCircle* loadingCircle = nullptr;
    GJCommentListLayer* listLayer = nullptr;
    cocos2d::CCMenu* buttonMenu;
    Ref<CCMenuItemSpriteExtra> clearSearchButton, settingsButton, inviteButton;
    cocos2d::CCNode* roomIdButton = nullptr;

    cocos2d::CCMenu* roomBtnMenu = nullptr;
    bool isWaiting = false;

    bool setup() override;
    void update(float) override;
    void onLoaded(bool stateChanged);
    void removeLoadingCircle();
    void addButtons();
    bool isLoading();
    void sortPlayerList();
    void applyFilter(const std::string_view input);
    void setRoomTitle(std::string name, uint32_t id);
    void onCopyRoomId(cocos2d::CCObject*);
    void recreateInviteButton();
};