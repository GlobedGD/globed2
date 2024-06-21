#pragma once
#include "Geode/binding/CCMenuItemToggler.hpp"
#include <defs/all.hpp>
#include <data/types/gd.hpp>

class RoomLayer : public cocos2d::CCLayer {
public:
    static RoomLayer* create();

    void reloadPlayerList(bool sendPacket = true);

    cocos2d::CCSize popupSize;
    float listWidth, listHeight;
    cocos2d::CCSize targetButtonSize;

protected:
    std::vector<PlayerRoomPreviewAccountData> playerList;
    std::vector<PlayerRoomPreviewAccountData> filteredPlayerList;

    LoadingCircle* loadingCircle = nullptr;
    GJCommentListLayer* listLayer = nullptr;
    cocos2d::CCMenu* buttonMenu;
    Ref<CCMenuItemSpriteExtra> clearSearchButton, settingsButton, inviteButton, refreshButton;
    Ref<CCMenuItemToggler> statusButton;
    cocos2d::CCNode* roomIdButton = nullptr;

    cocos2d::CCMenu* roomBtnMenu = nullptr;
    bool isWaiting = false;

    bool init() override;
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
    void onChangeStatus(cocos2d::CCObject*);
};