#include "visual_player.hpp"

#include "remote_player.hpp"
#include <util/misc.hpp>

using namespace geode::prelude;

bool VisualPlayer::init(RemotePlayer* parent, bool isSecond) {
    if (!CCNode::init() || !BaseVisualPlayer::init(parent, isSecond)) return false;

    this->playLayer = PlayLayer::get();

    auto& data = parent->getAccountData();

    Build<SimplePlayer>::create(data.icons.cube)
        .playerFrame(data.icons.cube, IconType::Cube)
        .store(playerIcon)
        .parent(this);

    Build<CCLabelBMFont>::create(data.name.c_str(), "chatFont.fnt")
        .store(playerName)
        .pos(0.f, 25.f)
        .parent(this);

    this->updateIcons(data.icons);

    return true;
}

void VisualPlayer::updateIcons(const PlayerIconData& icons) {
    auto* gm = GameManager::get();

    this->updateIconType(playerIconType);
    playerIcon->setColor(gm->colorForIdx(icons.color1));
    playerIcon->setSecondColor(gm->colorForIdx(icons.color2));

    if (icons.glowColor != -1) {
        playerIcon->m_hasGlowOutline = true;
        playerIcon->enableCustomGlowColor(gm->colorForIdx(icons.glowColor));
    } else {
        playerIcon->m_hasGlowOutline = false;
        playerIcon->disableCustomGlowColor();
    }
}

void VisualPlayer::updateData(const SpecificIconData& data, bool isDead) {
    this->setPosition(data.position);
    playerIcon->setRotation(data.rotation);
    playerIcon->setFlipX(data.isLookingLeft);

    PlayerIconType iconType = data.iconType;
    // in platformer, jetpack is serialized as ship so we make sure to show the right icon
    if (iconType == PlayerIconType::Ship && playLayer->m_level->isPlatformer()) {
        iconType = PlayerIconType::Jetpack;
    }

    if (data.iconType != playerIconType) {
        this->updateIconType(data.iconType);
    }

    this->setVisible(data.isVisible);
}

void VisualPlayer::updateName() {
    playerName->setString(parent->getAccountData().name.c_str());
    auto& sud = parent->getAccountData().specialUserData;
    sud.has_value() ? playerName->setColor(sud->nameColor) : playerName->setColor({255, 255, 255});
}

void VisualPlayer::updateIconType(PlayerIconType newType) {
    playerIconType = newType;

    const auto& accountData = parent->getAccountData();
    const auto& icons = accountData.icons;
    int newIcon = this->getIconWithType(icons, playerIconType);

    playerIcon->updatePlayerFrame(newIcon, util::misc::convertEnum<IconType>(newType));
}

void VisualPlayer::playDeathEffect() {
    GLOBED_UNIMPL("VisualPlayer::playDeathEffect")
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