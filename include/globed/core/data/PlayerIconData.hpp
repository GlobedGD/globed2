#pragma once
#include <stdint.h>
#include <Geode/binding/GameManager.hpp>

namespace globed {

constexpr uint16_t NO_GLOW = 65535;
constexpr uint8_t NO_TRAIL = 255;
constexpr uint8_t DEFAULT_DEATH = 1;

template <typename T, T None = std::numeric_limits<T>::max()>
struct ColorId {
    ColorId(T val) : value(val) {}
    ColorId() : value(None) {}

    ColorId& operator=(T val) {
        this->value = val;
        return *this;
    }

    cocos2d::ccColor3B asColor() const {
        return GameManager::get()->colorForIdx(this->asIdx());
    }

    T inner() const {
        return value;
    }

    int asIdx() const {
        return this->isNone() ? (int)-1 : (int)value;
    }

    bool isNone() const {
        return value == None;
    }

private:
    T value;
};

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
};

}
