#pragma once
#include "VisualPlayer.hpp"
#include "ProgressArrow.hpp"
#include "ProgressIcon.hpp"

#include <cocos2d.h>

namespace globed {

struct GameCameraState;
struct OutFlags;

class GLOBED_DLL RemotePlayer {
public:
    RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, cocos2d::CCNode* parentNode);
    ~RemotePlayer();

    void update(const PlayerState& state, const GameCameraState& camState, const OutFlags& flags, bool forceHide = false);
    void handleDeath(const PlayerDeath& death);
    void handleSpiderTp(const SpiderTeleportData& tp, bool p1);
    bool isDataInitialized() const;
    bool isTeamInitialized() const;
    void initData(const PlayerDisplayData& data, uint16_t teamId = 0xffff);
    void updateTeam(uint16_t teamId);
    bool isTeammate(bool whatWhenNoTeams = true);

    void setForceHide(bool hide);

    VisualPlayer* player1();
    VisualPlayer* player2();

    PlayerDisplayData& displayData();

private:
    friend class VisualPlayer;

    PlayerState m_state;
    PlayerDisplayData m_data;
    bool m_dataInitialized = false;
    bool m_forceHide = false;
    cocos2d::CCNode* m_parentNode = nullptr;
    VisualPlayer* m_player1 = nullptr;
    VisualPlayer* m_player2 = nullptr;
    ProgressArrow* m_progArrow = nullptr;
    ProgressIcon* m_progIcon = nullptr;
    std::optional<uint16_t> m_teamId;
};

}