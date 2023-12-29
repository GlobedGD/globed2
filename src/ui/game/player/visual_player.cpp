#include "visual_player.hpp"

#include <UIBuilder.hpp>

#include "remote_player.hpp"
#include <util/misc.hpp>

using namespace geode::prelude;

bool VisualPlayer::init(RemotePlayer* parent, bool isSecond) {
    if (!CCNode::init()) return false;
    this->parent = parent;
    this->isSecond = isSecond;

    auto& data = parent->getAccountData();

    Build<SimplePlayer>::create(data.icons.cube)
        .playerFrame(data.icons.cube, IconType::Cube)
        .store(playerIcon)
        .parent(this);

    this->updateIcons(data.icons);

    return true;
}

void VisualPlayer::updateIcons(const PlayerIconData& icons) {
    auto* gm = GameManager::get();

    playerIcon->setColor(gm->colorForIdx(icons.color1));
    playerIcon->setSecondColor(gm->colorForIdx(icons.color2));

    if (icons.glowColor != -1) {
        playerIcon->m_hasGlowOutline = true;
        playerIcon->enableCustomGlowColor(gm->colorForIdx(icons.glowColor));
    } else {
        playerIcon->m_hasGlowOutline = false;
        playerIcon->disableCustomGlowColor();
    }

    playerIcon->updateColors();
}

void VisualPlayer::updateData(const SpecificIconData& data) {
    this->setPosition(data.position);
    this->setRotation(data.rotation);

    if (data.iconType != playerIconType) {
        this->updateIconType(data.iconType);
    }
}

void VisualPlayer::updateIconType(PlayerIconType newType) {
    playerIconType = newType;
    int newIcon = 1;

    const auto& accountData = parent->getAccountData();
    const auto& icons = accountData.icons;

    switch (newType) {
        case PlayerIconType::Cube: newIcon = icons.cube; break;
        case PlayerIconType::Ship: newIcon = icons.ship; break;
        case PlayerIconType::Ball: newIcon = icons.ball; break;
        case PlayerIconType::Ufo: newIcon = icons.ufo; break;
        case PlayerIconType::Wave: newIcon = icons.wave; break;
        case PlayerIconType::Robot: newIcon = icons.robot; break;
        case PlayerIconType::Spider: newIcon = icons.spider; break;
        case PlayerIconType::Swing: newIcon = icons.swing; break;
        default: newIcon = icons.cube; break;
    };

    playerIcon->updatePlayerFrame(newIcon, util::misc::convertEnum<IconType>(newType));
}

VisualPlayer* VisualPlayer::create(RemotePlayer* parent, bool isSecond) {
    auto* ret = new VisualPlayer;
    if (ret->init(parent, isSecond)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}