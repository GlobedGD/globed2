#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class PlayerListCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 30.0f;

    static PlayerListCell* create(const PlayerRoomPreviewAccountData& data, float width, bool forInviting);

protected:
    bool init(const PlayerRoomPreviewAccountData& data, float width, bool forInviting);
    void onOpenProfile(cocos2d::CCObject*);

    void createInviteButton();
    void createJoinButton();
    void createAdminButton();

    void sendInvite();
    void enableInvites();

    PlayerRoomPreviewAccountData data;
    float width;

    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra *playButton = nullptr, *inviteButton = nullptr;
    GlobedSimplePlayer* simplePlayer;

    bool isFriend;
};