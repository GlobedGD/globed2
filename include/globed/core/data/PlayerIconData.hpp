#pragma once
#include <stdint.h>

namespace globed {

constexpr uint16_t NO_GLOW = 65535;
constexpr uint8_t NO_TRAIL = 255;
constexpr uint8_t DEFAULT_DEATH = 1;

struct PlayerIconData {
    int16_t cube;
    int16_t ship;
    int16_t ball;
    int16_t ufo;
    int16_t wave;
    int16_t robot;
    int16_t spider;
    int16_t swing;
    int16_t jetpack;
    uint16_t color1;
    uint16_t color2;
    uint16_t glowColor = NO_GLOW;
    uint8_t deathEffect = DEFAULT_DEATH;
    uint8_t trail = NO_TRAIL;
    uint8_t shipTrail = NO_TRAIL;
};

}
