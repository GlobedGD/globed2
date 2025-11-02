#include <globed/core/game/VisualPlayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/util/lazy.hpp>
#include <core/PreloadManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <ui/misc/NameLabel.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

static constexpr int ROBOT_FIRE_ACTION = 1325385193;
static constexpr float NAME_OFFSET = 28.f;
static constexpr float STATUS_ICONS_OFFSET = NAME_OFFSET + 15.f;

using namespace geode::prelude;

namespace globed {

#ifdef GLOBED_DEBUG
static inline bool lerpDebug() {
    static bool val = Loader::get()->getLaunchFlag("globed/core.dev.lerp-debug");
    return val;
}
#else
static inline bool lerpDebug() {
    return false;
}
#endif

static inline bool hideNearby(GJBaseGameLayer* gjbgl) {
    return setting<bool>(gjbgl->m_level->isPlatformer() ? "core.player.hide-nearby-plat" : "core.player.hide-nearby-classic");
}

bool VisualPlayer::init(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond) {
    if (!PlayerObject::init(1, 1, gameLayer, gameLayer->m_objectLayer, !gameLayer->m_isEditor)) {
        return false;
    }

    m_remotePlayer = rp;
    m_isSecond = isSecond;
    m_isEditor = gameLayer->m_isEditor;
    m_isPlatformer = gameLayer->m_level->isPlatformer();

    // preload the cube icon so the passengers are correct
    this->updateIconType(PlayerIconType::Cube);
    m_prevMode = PlayerIconType::Cube;

    // create the name label
    bool showName = setting<bool>("core.player.show-names") && (!isSecond || setting<bool>("core.player.dual-name"));
    m_forceHideName = !showName;

    m_nameLabel = Build<NameLabel>::create("", "chatFont.fnt")
        .visible(showName)
        .pos(0.f, NAME_OFFSET)
        .parent(playerNode)
        .store(m_nameLabel);

    m_nameLabel->setShadowEnabled(true);

    if (!isSecond && setting<bool>("core.player.status-icons")) {
        float opacity = static_cast<unsigned char>(setting<float>("core.player.opacity") * 255.f);
        m_statusIcons = Build<PlayerStatusIcons>::create(opacity)
            .scale(0.8f)
            .anchorPoint(0.5f, 0.f)
            .pos(0.f, showName ? STATUS_ICONS_OFFSET : NAME_OFFSET)
            .parent(playerNode)
            .id("status-icons"_spr);
    }

    if (lerpDebug()) {
        Build<CCDrawNode>::create()
            .id(fmt::format("debug-trajectory"_spr).c_str())
            .parent(gameLayer->m_objectLayer)
            .store(m_playerTrajectory);

        m_playerTrajectory->m_bUseArea = false;
    }

    this->updateOpacity();

    return true;
}

void VisualPlayer::updateFromData(const PlayerObjectData& data, const PlayerState& state, const GameCameraState& camState, bool forceHide) {
    if (lerpDebug()) {
        this->updateLerpTrajectory(data);
    }

    m_prevRotating = data.isRotating;
    m_prevPosition = data.position;
    m_prevRotation = data.rotation;

    // set some PlayerObject members

    m_isGoingLeft = data.isLookingLeft;
    m_isDead = state.isDead;
    m_isUpsideDown = data.isUpsideDown;
    m_isOnGround = data.isGrounded;
    // m_isRotating = data.isRotating;
    // m_isSideways = data.isSideways;

    // m_isShip = data.iconType == PlayerIconType::Ship;
    // m_isBall = data.iconType == PlayerIconType::Ball;
    // m_isBird = data.iconType == PlayerIconType::Ufo;
    // m_isDart = data.iconType == PlayerIconType::Wave;
    // m_isRobot = data.iconType == PlayerIconType::Robot;
    // m_isSpider = data.iconType == PlayerIconType::Spider;
    // m_isSwing = data.iconType == PlayerIconType::Swing;

    if (data.extData) {
        auto& ed = *data.extData;
        m_platformerXVelocity = ed.velocityX;
        m_yVelocity = ed.velocityY;
        m_isAccelerating = ed.accelerating;
        m_accelerationOrSpeed = ed.acceleration;
        m_fallStartY = ed.fallStartY;
        m_isOnGround2 = ed.isOnGround2;
        m_gravityMod = ed.gravityMod;
        m_gravity = ed.gravity;
        m_touchedPad = ed.touchedPad;
    }

    // calculate visibility n stuff

    bool isNearby = this->isPlayerNearby(data, camState);

    bool cameNearby = isNearby && !m_prevNearby;
    m_prevNearby = isNearby;

    // determine if the player should be visible
    bool shouldBeVisible = true;

    if (forceHide) {
        shouldBeVisible = false;
    } else if (state.isPracticing && setting<bool>("core.player.hide-practicing")) {
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
        this->setPosition(data.position);
        this->setRotation(data.rotation);
        m_mainLayer->setRotation(innerRot);

        // rotate the name label together with the camera
        bool rotateNames = setting<bool>("core.player.rotate-names");
        CameraDirection dir{};

        if (rotateNames && *gjbgl) {
            dir = gjbgl->getCameraDirection();
        }

        m_nameLabel->setPosition(data.position + dir.vector * NAME_OFFSET);
        m_nameLabel->setRotation(dir.angle);

        if (m_emoteBubble && m_emoteBubble->isPlaying()) {
            float yoff = (m_nameLabel->isVisible() ? STATUS_ICONS_OFFSET : NAME_OFFSET) - 3.f;
            float xoff = 25.f;
            CCPoint dirVecPerp{dir.vector.y, -dir.vector.x};
            CCPoint fullOffset = dir.vector * yoff + dirVecPerp * xoff;

            m_emoteBubble->setPosition(data.position + fullOffset);
            m_emoteBubble->setRotation(dir.angle);
        }

        if (m_statusIcons) {
            m_statusIcons->setPosition(data.position + dir.vector * (m_nameLabel->isVisible() ? STATUS_ICONS_OFFSET : NAME_OFFSET));
            m_statusIcons->setRotation(dir.angle);
        }
    }

    if (data.isRotating || distanceTo90deg > 1.f) {
        this->cancelPlatformerJumpAnim();
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

    if ((switchedMode || (isNearby && hideNearby(*gjbgl))) && !updatedOpacity) {
        this->updateOpacity();
        updatedOpacity = true;
    }

    if (m_statusIcons && shouldBeVisible) {
        PlayerStatusFlags flags = {};
        flags.paused = state.isPaused;
        flags.practicing = state.isPracticing;
        flags.speakingMuted = false;
        flags.editing = state.isInEditor;
        // flags.speaking = ;
        // flags.loudness = ;
        m_statusIcons->updateStatus(flags);
    }

    // TODO (low): dashing

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
        m_swingFireTop->setVisible(false);
        m_swingFireMiddle->setVisible(false);
        m_swingFireBottom->setVisible(false);
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

void VisualPlayer::updateLerpTrajectory(const PlayerObjectData& data) {
    if (!m_playerTrajectory) {
        return;
    }

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
            m_isSecond ? (newstate.player2 ? newstate.player2->position : CCPoint{}) : newstate.player1->position,
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

PlayerIconData& VisualPlayer::icons() {
    return m_remotePlayer->m_data.icons;
}

PlayerDisplayData& VisualPlayer::displayData() {
    return m_remotePlayer->m_data;
}

void VisualPlayer::updateOpacity() {
    float mult = 1.f;

    bool hideNearby_ = hideNearby(GlobedGJBGL::get(m_gameLayer));

    if (hideNearby_) {
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
    if (hideNearby_) {
        m_nameLabel->updateOpacity(opacity);

        if (m_emoteBubble) {
            m_emoteBubble->setOpacity(opacity);
        }

        if (m_statusIcons) {
            m_statusIcons->setOpacity(opacity);
        }
    }
}

void VisualPlayer::updateIconType(PlayerIconType iconType) {
    auto& icons = this->icons();

    this->toggleFlyMode(false, true);
    this->toggleRollMode(false, false);
    this->toggleBirdMode(false, false);
    this->toggleDartMode(false, false);
    this->toggleRobotMode(false, false);
    this->toggleSpiderMode(false, false);
    this->toggleSwingMode(false, false);

    switch (iconType) {
        case PlayerIconType::Unknown: break;
        case PlayerIconType::Cube: {
            // don't call toggle
            this->updatePlayerFrame(icons.cube);
        } break;
        case PlayerIconType::Ship: {
            this->toggleFlyMode(true, false);
            this->updatePlayerShipFrame(icons.ship);
        } break;
        case PlayerIconType::Ball: {
            this->toggleRollMode(true, false);
            this->updatePlayerRollFrame(icons.ball);
        } break;
        case PlayerIconType::Ufo: {
            this->toggleBirdMode(true, false);
            this->updatePlayerBirdFrame(icons.ufo);
        } break;
        case PlayerIconType::Wave: {
            this->toggleDartMode(true, false);
            this->updatePlayerDartFrame(icons.wave);
        } break;
        case PlayerIconType::Robot: {
            this->toggleRobotMode(true, false);
            this->updatePlayerRobotFrame(icons.robot);
        } break;
        case PlayerIconType::Spider: {
            this->toggleSpiderMode(true, false);
            this->updatePlayerSpiderFrame(icons.spider);
        } break;
        case PlayerIconType::Swing: {
            this->toggleSwingMode(true, false);
            this->updatePlayerSwingFrame(icons.swing);
        } break;
        case PlayerIconType::Jetpack: {
            this->toggleFlyMode(true, true);
            this->updatePlayerJetpackFrame(icons.jetpack);
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

    // custom nodes
    $clear(m_nameLabel);
    $clear(m_statusIcons);
    $clear(m_emoteBubble);
    $clear(m_playerTrajectory);

#undef $clear
}

void VisualPlayer::updateDisplayData() {
    auto gm = cachedSingleton<GameManager>();
    auto& rm = RoomManager::get();
    auto& ddata = this->displayData();

    // update the team if not initialized
    if (!m_teamInitialized) {
        if (auto id = rm.getTeamIdForPlayer(ddata.accountId)) {
            this->updateTeam(*id);
        }
    }

    m_nameLabel->updateName(ddata.username);
    m_nameLabel->updateOpacity(globed::setting<float>("core.player.name-opacity"));
    if (ddata.specialUserData) {
        m_nameLabel->updateWithRoles(*ddata.specialUserData);
    }

    if (globed::setting<bool>("core.player.default-death-effects")) {
        ddata.icons.deathEffect = DEFAULT_DEATH;
    }

    this->updatePlayerObjectIcons(true);
    this->updateIconType(m_prevMode);
}

void VisualPlayer::updateTeam(uint16_t teamId) {
    if (auto team = RoomManager::get().getTeam(teamId)) {
        m_nameLabel->updateTeam(teamId, team->color);
        m_teamInitialized = true;
    }
}

void VisualPlayer::playDeathEffect() {
    auto* gm = globed::cachedSingleton<GameManager>();

    int oldEffect = gm->getPlayerDeathEffect();
    gm->setPlayerDeathEffect(this->icons().deathEffect);

    // prevent a crash here if somehow death effects arent preloaded
    if (this->icons().deathEffect != 1 && !PreloadManager::get().deathEffectsLoaded()) {
        gm->setPlayerDeathEffect(1);
    }

    PlayerObject::playDeathEffect();

    gm->setPlayerDeathEffect(oldEffect);
}

void VisualPlayer::handleSpiderTp(const SpiderTeleportData& tp) {
    auto pl = globed::cachedSingleton<GameManager>()->m_playLayer;
    if (!pl || !m_prevNearby) return;

    m_playEffects = true;
    this->stopActionByTag(SPIDER_TELEPORT_COLOR_ACTION);

    auto arr = pl->m_circleWaveArray;
    size_t countBefore = arr ? arr->count() : 0;
    this->playSpiderDashEffect(tp.from, tp.to);
    size_t countAfter = arr ? arr->count() : 0;

    for (size_t i = countBefore; i < countAfter; i++) {
        static_cast<CCNode*>(arr->objectAtIndex(i))->setTag(SPIDER_DASH_CIRCLE_WAVE_TAG);
    }

    for (auto child : m_parentLayer->getChildrenExt()) {
        if (child->getZOrder() != 40) continue;
        if (!child->getID().empty()) continue;

        auto sprite = typeinfo_cast<CCSprite*>(child);
        if (!sprite) continue;

        auto sfc = cachedSingleton<CCSpriteFrameCache>();
        auto* spdash1 = sfc->spriteFrameByName("spiderDash_001.png")->getTexture();
        auto* tex = sprite->getTexture();

        if (tex == spdash1) {
            sprite->setTag(SPIDER_DASH_SPRITE_TAG);
        }
    }

    m_tpColorDelta = 0.f;
    this->spiderTeleportUpdateColor();
}

static inline ccColor3B lerpColor(ccColor3B from, ccColor3B to, float delta) {
    delta = std::clamp(delta, 0.f, 1.f);

    ccColor3B out;
    out.r = std::lerp(from.r, to.r, delta);
    out.g = std::lerp(from.g, to.g, delta);
    out.b = std::lerp(from.b, to.b, delta);

    return out;
}

void VisualPlayer::spiderTeleportUpdateColor() {
    constexpr float MAX_TIME = 0.4f;

    m_tpColorDelta += (1.f / 60.f);

    float delta = m_tpColorDelta / MAX_TIME;

    if (delta >= 1.f) {
        this->stopActionByTag(SPIDER_TELEPORT_COLOR_ACTION);
        this->setColor(m_color1);
        this->setSecondColor(m_color2);
        return;
    }

    auto main = lerpColor(ccColor3B{255, 255, 255}, m_color1, delta);
    auto secondary = lerpColor(ccColor3B{255, 255, 255}, m_color2, delta);

    this->setColor(main);
    this->setSecondColor(secondary);

    auto* seq = CCSequence::create(
        CCDelayTime::create(1.f / 60.f),
        CCCallFunc::create(this, callfunc_selector(VisualPlayer::spiderTeleportUpdateColor)),
        nullptr
    );
    seq->setTag(SPIDER_TELEPORT_COLOR_ACTION);

    this->runAction(seq);
}

void VisualPlayer::playPlatformerJump() {
    if (!m_prevNearby) return;

    if (m_isPlatformer && m_prevMode == PlayerIconType::Cube && !m_prevRotating) {
        this->animatePlatformerJump(1.f);
        m_didPlatformerJump = true;
    }
}

void VisualPlayer::playEmote(uint32_t emoteId) {
    if (!m_emoteBubble) {
        m_emoteBubble = Build<EmoteBubble>::create()
            .parent(m_remotePlayer->m_parentNode);
    }

    m_emoteBubble->playEmote(emoteId);
}

void VisualPlayer::cancelPlatformerJumpAnim() {
    if (m_didPlatformerJump) {
        m_didPlatformerJump = false;
        this->stopPlatformerJumpAnimation();
    }
}

void VisualPlayer::updatePlayerObjectIcons(bool skipFrames) {
    auto* gm = globed::cachedSingleton<GameManager>();
    auto& icons = this->icons();

    m_color1 = icons.color1.asColor();
    m_color2 = icons.color2.asColor();

    this->setColor(m_color1);
    this->setSecondColor(m_color2);

    if (!icons.glowColor.isNone()) {
        this->m_hasGlow = true;
        this->enableCustomGlowColor(icons.glowColor.asColor());
    } else {
        this->m_hasGlow = false;
        this->disableCustomGlowColor();
    }

    if (!skipFrames) {
        this->updatePlayerFrame(icons.cube);
        this->updatePlayerShipFrame(icons.ship);
        this->updatePlayerRollFrame(icons.ball);
        this->updatePlayerBirdFrame(icons.ufo);
        this->updatePlayerDartFrame(icons.wave);
        this->updatePlayerRobotFrame(icons.robot);
        this->updatePlayerSpiderFrame(icons.spider);
        this->updatePlayerSwingFrame(icons.swing);
        this->updatePlayerJetpackFrame(icons.jetpack);
    }

    this->updateGlowColor();
    this->updatePlayerGlow();

    // set opacities
    this->updateOpacity();
}

bool VisualPlayer::isPlayerNearby(const PlayerObjectData& data, const GameCameraState& camState) {
    // always render them in editor (cause im lazy)
    if (m_isEditor) return true;

    // check if they are inside a grid of 3x3 screens
    float fullScaleMult = 3.f;
    float originMoveMult = (fullScaleMult - 1.f) / 2.f; // magic

    CCSize origCoverage = camState.cameraCoverage();
    CCSize cameraCoverage = origCoverage * fullScaleMult;
    CCPoint cameraOrigin = camState.cameraOrigin - origCoverage * originMoveMult;

    float cameraLeft = cameraOrigin.x;
    float cameraRight = cameraOrigin.x + cameraCoverage.width;
    float cameraBottom = cameraOrigin.y;
    float cameraTop = cameraOrigin.y + cameraCoverage.height;

    auto& pos = data.position;

    return pos.x >= cameraLeft && pos.x <= cameraRight &&
           pos.y >= cameraBottom && pos.y <= cameraTop;
}

CCPoint VisualPlayer::getLastPosition() {
    return m_prevPosition;
}

float VisualPlayer::getLastRotation() {
    return m_prevRotation;
}

void VisualPlayer::setVisible(bool vis) {
    if (vis == m_bVisible) return;
    PlayerObject::setVisible(vis);

    m_nameLabel->setVisible(vis && !m_forceHideName);
    if (m_statusIcons) m_statusIcons->setVisible(vis);
    if (m_emoteBubble) m_emoteBubble->setVisible(vis);
}

VisualPlayer* VisualPlayer::create(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond) {
    auto ret = new VisualPlayer();
    if (ret->init(gameLayer, rp, playerNode, isSecond)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
