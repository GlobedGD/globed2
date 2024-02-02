#pragma once
#include <data/types/game.hpp>

struct VisualPlayerState {
    SpecificIconData player1;
    SpecificIconData player2;
    bool isDead;
    bool isPaused;
    bool isPracticing;
};