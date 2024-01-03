#include "misc.hpp"
#include <data/types/game.hpp>

namespace util::misc {
    // IconType -> PlayerIconType
    template<> PlayerIconType convertEnum<PlayerIconType, IconType>(IconType value) {
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
    template<> IconType convertEnum<IconType, PlayerIconType>(PlayerIconType value) {
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
}