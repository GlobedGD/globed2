#pragma once
#include <defs.hpp>

#include "visual_player.hpp"
#include "complex_visual_player.hpp"
#include <data/types/gd.hpp>
#include <game/visual_state.hpp>

class RemotePlayer : public cocos2d::CCNode {
public:
    bool init(const PlayerAccountData& data);
    void updateAccountData(const PlayerAccountData& data);
    const PlayerAccountData& getAccountData() const;

    void updateData(const VisualPlayerState& data);

    unsigned int getDefaultTicks();
    void setDefaultTicks(unsigned int ticks);
    void incDefaultTicks();

    bool isValidPlayer();

    static RemotePlayer* create(const PlayerAccountData& data);
    static RemotePlayer* create();
protected:
    BaseVisualPlayer* player1;
    BaseVisualPlayer* player2;
    unsigned int defaultTicks = 0;

    PlayerAccountData accountData;
};