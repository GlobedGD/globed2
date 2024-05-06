#pragma once
#include <defs/all.hpp>
#include <data/types/gd.hpp>

#include <ui/general/simple_player.hpp>

class PlayerInviteCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 30.0f;

    static PlayerInviteCell* create(const PlayerPreviewAccountData& data);

protected:
    bool init(const PlayerPreviewAccountData& data);
    void onOpenProfile(cocos2d::CCObject*);

    PlayerPreviewAccountData data;

    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra* inviteButton;
    GlobedSimplePlayer* simplePlayer;
};