#include "complex_visual_player.hpp"

#include "remote_player.hpp"
#include <managers/settings.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool ComplexVisualPlayer::init(RemotePlayer* parent, bool isSecond) {
    if (!CCNode::init() || !BaseVisualPlayer::init(parent, isSecond)) return false;

    this->playLayer = PlayLayer::get();

    auto& data = parent->getAccountData();

    auto& settings = GlobedSettings::get();

    playerIcon = static_cast<ComplexPlayerObject*>(Build<PlayerObject>::create(1, 1, this->playLayer, this->playLayer->m_objectLayer, false)
        .opacity(static_cast<unsigned char>(settings.players.playerOpacity * 255.f))
        .parent(this)
        .collect());

    playerIcon->setRemoteState();

    Build<CCLabelBMFont>::create(data.name.c_str(), "chatFont.fnt")
        .opacity(static_cast<unsigned char>(settings.players.nameOpacity * 255.f))
        .visible(settings.players.showNames && (!isSecond || settings.players.dualName))
        .store(playerName)
        .pos(0.f, 25.f)
        .parent(this);

    this->updateIcons(data.icons);

    return true;
}

void ComplexVisualPlayer::updateIcons(const PlayerIconData& icons) {
    auto* gm = GameManager::get();
    auto& settings = GlobedSettings::get();

    playerIcon->togglePlatformerMode(playLayer->m_level->isPlatformer());

    storedIcons = icons;
    if (settings.players.defaultDeathEffect) {
        // set the default one.. (aka do nothing ig?)
    } else {
        // TODO set death effect somehow?
    }

    this->updatePlayerObjectIcons();
    this->updateIconType(playerIconType);
}

void ComplexVisualPlayer::updateData(const SpecificIconData& data) {
    this->setPosition(data.position);
    playerIcon->setRotation(data.rotation);
    // setFlipX doesnt work here for jetpack and stuff
    playerIcon->setScaleX(data.isLookingLeft ? -1.0f : 1.0f);
    playerIcon->setScaleY(data.isUpsideDown ? -1.0f : 1.0f);

    PlayerIconType iconType = data.iconType;
    // in platformer, jetpack is serialized as ship so we make sure to show the right icon
    if (iconType == PlayerIconType::Ship && playLayer->m_level->isPlatformer()) {
        iconType = PlayerIconType::Jetpack;
    }

    if (iconType != playerIconType) {
        this->updateIconType(iconType);
    }

    this->setVisible(data.isVisible);
}

void ComplexVisualPlayer::updateName() {
    playerName->setString(parent->getAccountData().name.c_str());
    auto& sud = parent->getAccountData().specialUserData;
    sud.has_value() ? playerName->setColor(sud->nameColor) : playerName->setColor({255, 255, 255});
}

void ComplexVisualPlayer::updateIconType(PlayerIconType newType) {
    PlayerIconType oldType = playerIconType;
    playerIconType = newType;

    const auto& accountData = parent->getAccountData();
    const auto& icons = accountData.icons;

    this->toggleAllOff();

    if (newType != PlayerIconType::Cube) {
        this->callToggleWith(newType, true, false);
    }

    this->callUpdateWith(newType, this->getIconWithType(icons, newType));
}

void ComplexVisualPlayer::playDeathEffect() {
    playerIcon->playDeathEffect();
}

void ComplexVisualPlayer::updatePlayerObjectIcons() {
    auto* gm = GameManager::get();

    playerIcon->setColor(gm->colorForIdx(storedIcons.color1));
    playerIcon->setSecondColor(gm->colorForIdx(storedIcons.color2));

    playerIcon->updatePlayerShipFrame(storedIcons.ship);
    playerIcon->updatePlayerRollFrame(storedIcons.ball);
    playerIcon->updatePlayerBirdFrame(storedIcons.ufo);
    playerIcon->updatePlayerDartFrame(storedIcons.wave);
    playerIcon->updatePlayerRobotFrame(storedIcons.robot);
    playerIcon->updatePlayerSpiderFrame(storedIcons.spider);
    playerIcon->updatePlayerSwingFrame(storedIcons.swing);
    playerIcon->updatePlayerJetpackFrame(storedIcons.jetpack);
    playerIcon->updatePlayerFrame(storedIcons.cube);

    if (storedIcons.glowColor != -1) {
        // playerIcon->m_hasGlowOutline = true;
        playerIcon->enableCustomGlowColor(gm->colorForIdx(storedIcons.glowColor));
    } else {
        // playerIcon->m_hasGlowOutline = false;
        playerIcon->disableCustomGlowColor();
    }

    playerIcon->updatePlayerGlow();
}

void ComplexVisualPlayer::toggleAllOff() {
    playerIcon->toggleFlyMode(false, false);
    playerIcon->toggleRollMode(false, false);
    playerIcon->toggleBirdMode(false, false);
    playerIcon->toggleDartMode(false, false);
    playerIcon->toggleRobotMode(false, false);
    playerIcon->toggleSpiderMode(false, false);
    playerIcon->toggleSwingMode(false, false);
}

void ComplexVisualPlayer::callToggleWith(PlayerIconType type, bool arg1, bool arg2) {
    switch (type) {
    case PlayerIconType::Ship: playerIcon->toggleFlyMode(arg1, arg2); break;
    case PlayerIconType::Ball: playerIcon->toggleRollMode(arg1, arg2); break;
    case PlayerIconType::Ufo: playerIcon->toggleBirdMode(arg1, arg2); break;
    case PlayerIconType::Wave: playerIcon->toggleDartMode(arg1, arg2); break;
    case PlayerIconType::Robot: playerIcon->toggleRobotMode(arg1, arg2); break;
    case PlayerIconType::Spider: playerIcon->toggleSpiderMode(arg1, arg2); break;
    case PlayerIconType::Swing: playerIcon->toggleSwingMode(arg1, arg2); break;
    case PlayerIconType::Jetpack: playerIcon->toggleFlyMode(arg1, arg2); break;
    default: break;
    }
}

void ComplexVisualPlayer::callUpdateWith(PlayerIconType type, int icon) {
    switch (type) {
    case PlayerIconType::Cube: playerIcon->updatePlayerFrame(icon); break;
    case PlayerIconType::Ship: playerIcon->updatePlayerShipFrame(icon); break;
    case PlayerIconType::Ball: playerIcon->updatePlayerRollFrame(icon); break;
    case PlayerIconType::Ufo: playerIcon->updatePlayerBirdFrame(icon); break;
    case PlayerIconType::Wave: playerIcon->updatePlayerDartFrame(icon); break;
    case PlayerIconType::Robot: playerIcon->updatePlayerRobotFrame(icon); break;
    case PlayerIconType::Spider: playerIcon->updatePlayerSpiderFrame(icon); break;
    case PlayerIconType::Swing: playerIcon->updatePlayerSwingFrame(icon); break;
    case PlayerIconType::Jetpack: playerIcon->updatePlayerJetpackFrame(icon); break;
    case PlayerIconType::Unknown: break;
    }
}

ComplexVisualPlayer* ComplexVisualPlayer::create(RemotePlayer* parent, bool isSecond) {
    auto ret = new ComplexVisualPlayer;
    if (ret->init(parent, isSecond)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}