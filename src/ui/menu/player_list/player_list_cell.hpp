#pragma once
#include <Geode/Geode.hpp>
#include <data/types/gd.hpp>

class PlayerListCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 45.0f;

    static PlayerListCell* create(const PlayerPreviewAccountData& data);

protected:
    bool init(const PlayerPreviewAccountData& data);
    void onOpenProfile(cocos2d::CCObject*);

    PlayerPreviewAccountData data;

    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra* btnOpenLevel;
    SimplePlayer* simplePlayer;
};