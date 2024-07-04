#include "into.hpp"

#include <defs/geode.hpp>
#include <data/types/game.hpp>

using namespace geode::prelude;

namespace globed {

// IconType -> PlayerIconType
template<> PlayerIconType into(const IconType& value) {
    switch (value) {
        case IconType::Cube: return PlayerIconType::Cube;
        case IconType::Ship: return PlayerIconType::Ship;
        case IconType::Ball: return PlayerIconType::Ball;
        case IconType::Ufo: return PlayerIconType::Ufo;
        case IconType::Wave: return PlayerIconType::Wave;
        case IconType::Robot: return PlayerIconType::Robot;
        case IconType::Spider: return PlayerIconType::Spider;
        case IconType::Swing: return PlayerIconType::Swing;
        case IconType::Jetpack: return PlayerIconType::Jetpack;
        default: return PlayerIconType::Cube;
    }
}

// PlayerIconType -> IconType
template<> IconType into(const PlayerIconType& value) {
    switch (value) {
        case PlayerIconType::Cube: return IconType::Cube;
        case PlayerIconType::Ship: return IconType::Ship;
        case PlayerIconType::Ball: return IconType::Ball;
        case PlayerIconType::Ufo: return IconType::Ufo;
        case PlayerIconType::Wave: return IconType::Wave;
        case PlayerIconType::Robot: return IconType::Robot;
        case PlayerIconType::Spider: return IconType::Spider;
        case PlayerIconType::Swing: return IconType::Swing;
        case PlayerIconType::Jetpack: return IconType::Jetpack;
        default: return IconType::Cube;
    }
}

// cc4b -> cc3b
template <>
ccColor3B into(const ccColor4B& value) {
    return ccColor3B {
        .r = value.r,
        .g = value.g,
        .b = value.b
    };
}

// cc3b -> cc4b
template <>
ccColor4B into(const ccColor3B& value) {
    return ccColor4B {
        .r = value.r,
        .g = value.g,
        .b = value.b,
        .a = 255
    };
}

}