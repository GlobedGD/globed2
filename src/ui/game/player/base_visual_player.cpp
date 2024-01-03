#include "base_visual_player.hpp"

bool BaseVisualPlayer::init(RemotePlayer* parent, bool isSecond) {
    this->parent = parent;
    this->isSecond = isSecond;
    return true;
}

int BaseVisualPlayer::getIconWithType(const PlayerIconData& icons, PlayerIconType type) {
    int newIcon = icons.cube;

    switch (type) {
        case PlayerIconType::Cube: newIcon = icons.cube; break;
        case PlayerIconType::Ship: newIcon = icons.ship; break;
        case PlayerIconType::Ball: newIcon = icons.ball; break;
        case PlayerIconType::Ufo: newIcon = icons.ufo; break;
        case PlayerIconType::Wave: newIcon = icons.wave; break;
        case PlayerIconType::Robot: newIcon = icons.robot; break;
        case PlayerIconType::Spider: newIcon = icons.spider; break;
        case PlayerIconType::Swing: newIcon = icons.swing; break;
        case PlayerIconType::Jetpack: newIcon = icons.jetpack;
        default: newIcon = icons.cube; break;
    };

    return newIcon;
}