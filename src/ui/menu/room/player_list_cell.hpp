#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class PlayerListCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 30.0f;

    static PlayerListCell* create(const PlayerRoomPreviewAccountData& data, bool forInviting);

protected:
    bool init(const PlayerRoomPreviewAccountData& data, bool forInviting);
    void onOpenProfile(cocos2d::CCObject*);

    void createInviteButton();
    void createJoinButton();

    PlayerRoomPreviewAccountData data;

    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra *playButton = nullptr, *inviteButton = nullptr;
    GlobedSimplePlayer* simplePlayer;
};