#pragma once
#include "PlayerState.hpp"
#include "VisualPlayer.hpp"
#include <cocos2d.h>

namespace globed {

class RemotePlayer {
public:
    RemotePlayer(int playerId, GJBaseGameLayer* gameLayer, cocos2d::CCNode* parentNode);
    ~RemotePlayer();

    void update(const PlayerState& state);

private:
    friend class VisualPlayer;

    PlayerState m_state;
    cocos2d::CCNode* m_parentNode = nullptr;
    VisualPlayer* m_player1 = nullptr;
    VisualPlayer* m_player2 = nullptr;
};

}