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

    bool isValidPlayer();

    static RemotePlayer* create(const PlayerAccountData& data);
    static RemotePlayer* create();
protected:
    VisualPlayer* player1;
    VisualPlayer* player2;

    PlayerAccountData accountData;
};