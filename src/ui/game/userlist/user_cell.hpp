#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>

class GlobedUserCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 45.f;

    void refreshData(const PlayerStore::Entry& entry);

    static GlobedUserCell* create(const PlayerStore::Entry& entry, const PlayerAccountData& data);

    int playerId;

private:
    cocos2d::CCLabelBMFont* percentageLabel;
    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra* blockButton = nullptr;
    PlayerStore::Entry _data;

    bool init(const PlayerStore::Entry& entry, const PlayerAccountData& data);
    void onOpenProfile(cocos2d::CCObject*);
    void makeBlockButton();
};