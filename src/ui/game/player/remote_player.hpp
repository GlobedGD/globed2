#pragma once
#include <defs.hpp>

#include "visual_player.hpp"
#include "complex_visual_player.hpp"
#include <ui/game/progress/progress_icon.hpp>
#include <data/types/gd.hpp>
#include <game/visual_state.hpp>

class RemotePlayer : public cocos2d::CCNode {
public:
    bool init(PlayerProgressIcon* progressIcon, const PlayerAccountData& data);
    void updateAccountData(const PlayerAccountData& data);
    const PlayerAccountData& getAccountData() const;

    void updateData(const VisualPlayerState& data, bool playDeathEffect);
    void updateProgressIcon();

    unsigned int getDefaultTicks();
    void setDefaultTicks(unsigned int ticks);
    void incDefaultTicks();

    bool isValidPlayer();

    static RemotePlayer* create(PlayerProgressIcon* progressIcon, const PlayerAccountData& data);
    static RemotePlayer* create(PlayerProgressIcon* progressIcon);

    Ref<PlayerProgressIcon> progressIcon;
protected:
    BaseVisualPlayer* player1;
    BaseVisualPlayer* player2;
    unsigned int defaultTicks = 0;
    float lastXPosition = 0.f;

    PlayerAccountData accountData;
};