#pragma once
#include <defs.hpp>

#include <data/types/gd.hpp>
#include "visual_player.hpp"

class RemotePlayer : public cocos2d::CCNode {
public:
    bool init(const PlayerAccountData& data);
    void updateAccountData(const PlayerAccountData& data);
    const PlayerAccountData& getAccountData() const;

    void updateData(const PlayerData& data);

    unsigned int getDefaultTicks();
    void setDefaultTicks(unsigned int ticks);
    void incDefaultTicks();

    bool isValidPlayer();

    static RemotePlayer* create(const PlayerAccountData& data);
    static RemotePlayer* create();
protected:
    VisualPlayer* player1;
    VisualPlayer* player2;
    unsigned int defaultTicks = 0;

    PlayerAccountData accountData;
};