#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class InviteNotification : public cocos2d::CCLayer {
public:
    static InviteNotification* create(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player);

protected:
    virtual bool init(uint32_t roomID, uint32_t roomToken, const PlayerRoomPreviewAccountData& player);

    cocos2d::CCMenu* menu;
    GlobedSimplePlayer* simplePlayer;
    CCMenuItemSpriteExtra* acceptBtn, rejectBtn;

};