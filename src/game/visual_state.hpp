#pragma once
#include <data/types/game.hpp>

struct VisualPlayerState {
    SpecificIconData player1 = {};
    SpecificIconData player2 = {};
    float currentPercentage = 0.f;
    bool isDead = false;
    bool isPaused = false;
    bool isPracticing = false;
    bool isDualMode = false;

    VisualPlayerState() {}

    VisualPlayerState(const PlayerData& pd) {
        player1 = pd.player1;
        player2 = pd.player2;
        currentPercentage = pd.currentPercentage;
        isDead = pd.isDead;
        isPaused = pd.isPaused;
        isPracticing = pd.isPracticing;
        isDualMode = pd.isDualMode;
    }
};