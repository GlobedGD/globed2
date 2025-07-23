#include <globed/core/game/VisualPlayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/lazy.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <UIBuilder.hpp>

static constexpr int ROBOT_FIRE_ACTION = 1325385193;

using namespace geode::prelude;

namespace globed {

bool VisualPlayer::init(GJBaseGameLayer* gameLayer, bool isSecond) {
    if (!PlayerObject::init(1, 1, gameLayer, gameLayer->m_objectLayer, gameLayer->m_isEditor)) {
        return false;
    }

    m_isSecond = isSecond;
    m_isEditor = gameLayer->m_isEditor;
    m_isPlatformer = gameLayer->m_level->isPlatformer();

    // TODO: name label, icons, status icons

    // preload the cube icon so the passengers are correct
    this->updateIconType(PlayerIconType::Cube);
    m_prevMode = PlayerIconType::Cube;

    this->updateOpacity();

#ifdef GLOBED_DEBUG_INTERPOLATION
    Build<CCDrawNode>::create()
        .id(fmt::format("debug-trajectory"_spr).c_str())
        .parent(gameLayer->m_objectLayer)
        .store(m_playerTrajectory);

    m_playerTrajectory->m_bUseArea = false;
#endif

    return true;
}

void VisualPlayer::updateFromData(const PlayerObjectData& data, const PlayerState& state) {
#ifdef GLOBED_DEBUG_INTERPOLATION
    if (m_playerTrajectory) {
        m_playerTrajectory->drawSegment(
            m_prevPosition, data.position,
            0.5f,
            ccColor4F{0.f, 1.f, 0.1f, 1.f}
        );

        auto& interpolator = GlobedGJBGL::get()->m_fields->m_interpolator;
        int accountId = m_remotePlayer->m_state.accountId;

        if (interpolator.hasPlayer(accountId)) {
            auto& newstate = interpolator.getNewerState(m_remotePlayer->m_state.accountId);

            m_playerTrajectory->drawCircle(
                m_isSecond ? (newstate.player2 ? newstate.player2->position : CCPoint{}) : newstate.player1.position,
                1.5f,
                ccColor4F{0.1f, 0.9f, 0.2f, 1.f},
                0.3f,
                ccColor4F{1.f, 0.f, 0.f, 0.f},
                8
            );
        }

        // detect if the player reset
        if (ccpDistance(m_prevPosition, data.position) > 50.f && data.position.x < m_prevPosition.x) {
            m_playerTrajectory->clear();
        }
    }
#endif

    m_prevRotating = data.isRotating;
    m_prevPosition = data.position;

    // TODO
    bool isNearby = true;

    bool cameNearby = isNearby && !m_prevNearby;
    m_prevNearby = isNearby;

    // determine if the player should be visible
    bool shouldBeVisible = true;

    if (state.isPracticing && setting<bool>("core.player.hide-practicing")) {
        shouldBeVisible = false;
    } else {
        shouldBeVisible = (data.isVisible || setting<bool>("core.player.force-visibility")) && isNearby;
    }

    this->setVisible(shouldBeVisible);
    if (!shouldBeVisible) {
        m_playEffects = false;
        if (m_regularTrail) m_regularTrail->setVisible(false);
        if (m_shipStreak) m_shipStreak->setVisible(false);
    }

    float innerRot = data.isSideways ? (data.isUpsideDown ? 90.f : -90.f) : 0.f;

    float distanceTo90deg = std::fmod(std::abs(data.rotation), 90.f);
    if (distanceTo90deg > 45.f) {
        distanceTo90deg = 90.f - distanceTo90deg;
    }

    auto gjbgl = GLOBED_LAZY(GlobedGJBGL::get());

    if (shouldBeVisible) {
        // TODO camera stuff ??
        this->setPosition(data.position);
        this->setRotation(data.rotation);
        m_mainLayer->setRotation(innerRot);

        // TODO: name label idk
    }

    if (data.isRotating || distanceTo90deg > 1.f) {
        // TODO: cancel platformer jump
    }

    bool updatedOpacity = false;
    if (!state.isDead && this->getOpacity() == 0) {
        this->updateOpacity();
        updatedOpacity = true;
    }

    m_startPosition = data.position;
    m_lastPosition = data.position;
    m_positionX = data.position.x;
    m_positionY = data.position.y;
    // TODO: should we set rest of the flags?

    // setFlipX doesn't work here for jetpack and stuff
    float mult = data.isMini ? 0.6f : 1.0f;
    this->setScaleX((data.isLookingLeft ? -1.0f : 1.0f) * mult);

    // swing is not flipped
    if (data.iconType == PlayerIconType::Swing) {
        this->setScaleY(mult);
    } else {
        this->setScaleY((data.isUpsideDown ? -1.0f : 1.0f) * mult);
    }

    m_scaleX = mult;
    m_scaleY = mult;

    bool switchedMode = data.iconType != m_prevMode;
    bool turningOffSwing = (data.iconType == PlayerIconType::Swing && switchedMode);
    bool turningOffRobot = (data.iconType == PlayerIconType::Robot && switchedMode);

    if (switchedMode) {
        this->updateIconType(data.iconType);
        m_prevMode = data.iconType;
    }

    if ((switchedMode || (isNearby && setting<bool>("core.player.hide-nearby"))) && !updatedOpacity) {
        this->updateOpacity();
        updatedOpacity = true;
    }

    // TODO: status icons

    // TODO: dashing

    // animate robot and spider
    if (data.iconType == PlayerIconType::Robot || data.iconType == PlayerIconType::Spider) {
        if (m_prevGrounded != data.isGrounded || m_prevStationary != data.isStationary || m_prevFalling != data.isFalling || switchedMode || cameNearby) {
            m_prevGrounded = data.isGrounded;
            m_prevStationary = data.isStationary;
            m_prevFalling = data.isFalling;

            if (shouldBeVisible) {
                data.iconType == PlayerIconType::Robot ?
                    this->updateRobotAnimation()
                    : this->updateSpiderAnimation();
            }
        }
    }
    // animate swing fire
    else if (data.iconType == PlayerIconType::Swing) {
        // if we just switched to swing, enable all fires
        if (switchedMode) {
            m_swingFireTop->setVisible(true);
            m_swingFireMiddle->setVisible(true);
            m_swingFireBottom->setVisible(true);

            m_swingFireMiddle->animateFireIn();
        }

        if (cameNearby || ((m_prevUpsideDown != data.isUpsideDown || switchedMode) && isNearby)) {
            m_prevUpsideDown = data.isUpsideDown;
            // now depending on the gravity, toggle either the bottom or top fire
            this->animateSwingFire(!m_prevUpsideDown);
        }
    }
    // remove swing fire
    else if (turningOffSwing) {
        // TODO: arent we immediately setting them to invisible??? the animateFireOut is useless??
        m_swingFireTop->setVisible(false);
        m_swingFireMiddle->setVisible(false);
        m_swingFireBottom->setVisible(false);

        if (isNearby) {
            m_swingFireTop->animateFireOut();
            m_swingFireMiddle->animateFireOut();
            m_swingFireBottom->animateFireOut();
        }
    }
    // remove robot fire
    else if (turningOffRobot) {
        if (shouldBeVisible) {
            this->animateRobotFire(false);
        } else {
            // just hide the fire
            this->hideRobotFire();
        }
    }

    if (m_prevPaused != state.isPaused) {
        m_prevPaused = state.isPaused;

        if (state.isPaused) {
            CCNode::onExit();
        } else {
            CCNode::onEnter();
        }
    }
}

void VisualPlayer::updateOpacity() {
    float mult = 1.f;

    bool hideNearby = setting<bool>("core.player.hide-nearby");

    if (hideNearby) {
        // calculate distance
        auto p1pos = m_gameLayer->m_player1->getPosition();
        auto p2pos = m_gameLayer->m_player2->getPosition();
        auto ourPos = m_prevPosition;

        float distance = std::min(
            cocos2d::ccpDistance(ourPos, p1pos),
            cocos2d::ccpDistance(ourPos, p2pos)
        );

        // range of 150 units (5 blocks)
        distance = std::clamp(distance, 0.f, 150.f);
        mult = distance / 150.f;
    }

    uint8_t opacity = static_cast<uint8_t>(setting<float>("core.player.opacity") * mult * 255.f);

    this->setOpacity(opacity);
    m_spiderSprite->GJRobotSprite::setOpacity(opacity);
    m_robotSprite->GJRobotSprite::setOpacity(opacity);
    if (m_shipStreak) {
        m_shipStreak->setOpacity(opacity);
    }
    if (m_regularTrail) {
        m_regularTrail->setOpacity(opacity);
    }

    // set name opacity as well if hide nearby is enabled
    if (hideNearby) {
        // TODO: no name label yet xd
    }
}

void VisualPlayer::updateIconType(PlayerIconType iconType) {
    // TODO: get icons
    this->toggleFlyMode(false, false);
    this->toggleRollMode(false, false);
    this->toggleBirdMode(false, false);
    this->toggleDartMode(false, false);
    this->toggleRobotMode(false, false);
    this->toggleSpiderMode(false, false);
    this->toggleSwingMode(false, false);

    // TODO: call updatexxx with icons
    switch (iconType) {
        case PlayerIconType::Unknown: break;
        case PlayerIconType::Cube: {
            // don't call toggle
        } break;
        case PlayerIconType::Ship: {
            this->toggleFlyMode(true, false);
        } break;
        case PlayerIconType::Ball: {
            this->toggleRollMode(true, false);
        } break;
        case PlayerIconType::Ufo: {
            this->toggleBirdMode(true, false);
        } break;
        case PlayerIconType::Wave: {
            this->toggleDartMode(true, false);
        } break;
        case PlayerIconType::Robot: {
            this->toggleRobotMode(true, false);
        } break;
        case PlayerIconType::Spider: {
            this->toggleSpiderMode(true, false);
        } break;
        case PlayerIconType::Swing: {
            this->toggleSwingMode(true, false);
        } break;
        case PlayerIconType::Jetpack: {
            this->toggleFlyMode(true, false);
        } break;
    }
}

void VisualPlayer::updateRobotAnimation() {
    if (m_prevGrounded && m_prevStationary) {
        // if on ground and not moving, play the idle animation
        m_robotSprite->tweenToAnimation("idle01", 0.1f);
        this->animateRobotFire(false);
    } else if (m_prevGrounded && !m_prevStationary) {
        // if on ground and moving, play the running animation
        m_robotSprite->tweenToAnimation("run", 0.1f);
        this->animateRobotFire(false);
    } else if (m_prevFalling) {
        // if in the air and falling, play falling animation
        m_robotSprite->tweenToAnimation("fall_loop", 0.1f);
        this->animateRobotFire(false);
    } else if (!m_prevFalling) {
        // if in the air and not falling, play jumping animation
        m_robotSprite->tweenToAnimation("jump_loop", 0.1f);
        this->animateRobotFire(true);
    }
}

void VisualPlayer::updateSpiderAnimation() {
    // this is practically the same as the robot animation

    if (!m_prevGrounded && m_prevFalling) {
        m_spiderSprite->tweenToAnimation("fall_loop", 0.1f);
    } else if (!m_prevGrounded && !m_prevFalling) {
        m_spiderSprite->tweenToAnimation("jump_loop", 0.1f);
    } else if (m_prevGrounded && m_prevStationary) {
        m_spiderSprite->tweenToAnimation("idle01", 0.1f);
    } else if (m_prevGrounded && !m_prevStationary) {
        m_spiderSprite->tweenToAnimation("run", 0.1f);
    }
}

void VisualPlayer::animateSwingFire(bool goingDown) {
    if (goingDown) {
        m_swingFireTop->animateFireIn();
        m_swingFireBottom->animateFireOut();
    } else {
        m_swingFireTop->animateFireOut();
        m_swingFireBottom->animateFireIn();
    }
}

void VisualPlayer::animateRobotFire(bool enable) {
    m_robotFire->stopActionByTag(ROBOT_FIRE_ACTION);

    CCSequence* seq;
    if (enable) {
        seq = CCSequence::create(
            CCDelayTime::create(0.15f),
            CCCallFunc::create(this, callfunc_selector(VisualPlayer::showRobotFire)),
            nullptr
        );

        m_robotFire->setVisible(true);
    } else {
        seq = CCSequence::create(
            CCDelayTime::create(0.1f),
            CCCallFunc::create(this, callfunc_selector(VisualPlayer::hideRobotFire)),
            nullptr
        );

        m_robotFire->animateFireOut();
    }

    seq->setTag(ROBOT_FIRE_ACTION);
    m_robotFire->runAction(seq);
}

void VisualPlayer::hideRobotFire() {
    m_robotFire->setVisible(false);
}

void VisualPlayer::showRobotFire() {
    m_robotFire->animateFireIn();
}

void VisualPlayer::cleanupObjectLayer() {
#define $clear(x) if (x) x->removeFromParent(); x = nullptr

    // Robtop does not properly remove most/all those nodes from the playerobject in the destructor,
    // so whenever someone leaves the level, these nodes are never deleted until you leave the level too.

    // Thanks sleepyut for finding this :)

    $clear(m_shipStreak);
    $clear(m_regularTrail);
    $clear(m_waveTrail);
    $clear(m_ghostTrail);

    $clear(m_playerGroundParticles);
    $clear(m_trailingParticles);
    $clear(m_shipClickParticles);
    $clear(m_vehicleGroundParticles);
    $clear(m_ufoClickParticles);
    $clear(m_robotBurstParticles);
    $clear(m_dashParticles);
    $clear(m_swingBurstParticles1);
    $clear(m_swingBurstParticles2);
    $clear(m_landParticles0);
    $clear(m_landParticles1);

    // hope i didnt forget anything..

#undef $clear
}

VisualPlayer* VisualPlayer::create(GJBaseGameLayer* gameLayer, bool isSecond) {
    auto ret = new VisualPlayer();
    if (ret->init(gameLayer, isSecond)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
