#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>
#include <game/player_store.hpp>
#include <ui/general/audio_visualizer.hpp>

class GlobedUserCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 40.f;

    void refreshData(const PlayerStore::Entry& entry);
    void updateVisualizer(float dt);

    static GlobedUserCell* create(const PlayerStore::Entry& entry, const PlayerAccountData& data);

    int playerId;
    int userId;
    std::string username;

private:
    cocos2d::CCLabelBMFont* percentageLabel;
    cocos2d::CCMenu* menu;
    CCMenuItemSpriteExtra *blockButton = nullptr, *kickButton = nullptr;
    cocos2d::CCMenu* buttonsWrapper = nullptr;
    GlobedAudioVisualizer* audioVisualizer = nullptr;
    PlayerStore::Entry _data;

    bool init(const PlayerStore::Entry& entry, const PlayerAccountData& data);
    void onOpenProfile(cocos2d::CCObject*);
    void makeBlockButton();
};