#pragma once
#include "VisualPlayer.hpp"
#include <globed/core/data/PlayerDisplayData.hpp>
#include <cocos2d.h>

namespace globed {

struct GameCameraState;

class RemotePlayer {
public:
    RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, cocos2d::CCNode* parentNode);
    ~RemotePlayer();

    void update(const PlayerState& state, const GameCameraState& camState);
    bool isDataInitialized() const;
    void initData(const PlayerDisplayData& data);

private:
    friend class VisualPlayer;

    PlayerState m_state;
    bool m_dataInitialized = false;
    cocos2d::CCNode* m_parentNode = nullptr;
    VisualPlayer* m_player1 = nullptr;
    VisualPlayer* m_player2 = nullptr;
};

}