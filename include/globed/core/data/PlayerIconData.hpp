#pragma once

#include "ColorId.hpp"

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
    ColorId<uint16_t> color1;
    ColorId<uint16_t> color2;
    ColorId<uint16_t> glowColor;
    uint8_t deathEffect = DEFAULT_DEATH;
    uint8_t trail = NO_TRAIL;
    uint8_t shipTrail = NO_TRAIL;

    static PlayerIconData getOwn();
};

}
