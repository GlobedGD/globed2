#pragma once
#include "VisualPlayer.hpp"
#include <globed/core/data/PlayerDisplayData.hpp>
#include <core/game/Interpolator.hpp>

#include <cocos2d.h>

namespace globed {

struct GameCameraState;

class RemotePlayer {
public:
    RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, cocos2d::CCNode* parentNode);
    ~RemotePlayer();

    void update(const PlayerState& state, const GameCameraState& camState);
    void handleDeath(const PlayerDeath& death);
    bool isDataInitialized() const;
    bool isTeamInitialized() const;
    void initData(const PlayerDisplayData& data, uint16_t teamId = 0xffff);
    void updateTeam(uint16_t teamId);
    bool isTeammate(bool whatWhenNoTeams = true);

    VisualPlayer* player1();
    VisualPlayer* player2();

    PlayerDisplayData& displayData();

private:
    friend class VisualPlayer;

    PlayerState m_state;
    PlayerDisplayData m_data;
    bool m_dataInitialized = false;
    cocos2d::CCNode* m_parentNode = nullptr;
    VisualPlayer* m_player1 = nullptr;
    VisualPlayer* m_player2 = nullptr;
    std::optional<uint16_t> m_teamId;
};

}