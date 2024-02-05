#pragma once
#include <defs.hpp>

#include <game/player_store.hpp>

class GlobedUserCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 30.f;

    void refreshData(const PlayerStore::Entry& entry);

    static GlobedUserCell* create(const PlayerStore::Entry& entry, const std::string_view name);

    int playerId;

private:
    cocos2d::CCLabelBMFont* nameLabel;
    PlayerStore::Entry _data;

    bool init(const PlayerStore::Entry& entry, const std::string_view name);
    void onOpenProfile(cocos2d::CCObject*);
};