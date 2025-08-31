#pragma once

#include <Geode/Geode.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/core/game/PlayerStatusIcons.hpp>
#include <core/game/Interpolator.hpp>
#include <ui/misc/NameLabel.hpp>

namespace globed {

static constexpr int ROBOT_FIRE_ACTION = 1000727;
static constexpr int SWING_FIRE_ACTION = 1000728;
static constexpr int SPIDER_TELEPORT_COLOR_ACTION = 1000729;
static constexpr int SPIDER_DASH_CIRCLE_WAVE_TAG = 234562345;
static constexpr int SPIDER_DASH_SPRITE_TAG = 234562347;
static constexpr int DEATH_EFFECT_TAG = 234562349;

class RemotePlayer;
struct GameCameraState;

class VisualPlayer : public PlayerObject {
public:
    static VisualPlayer* create(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond);
    void updateFromData(const PlayerObjectData& data, const PlayerState& state, const GameCameraState& camState);
    void cleanupObjectLayer();

    void updateDisplayData();
    void updateTeam(uint16_t teamId);
    void playDeathEffect();
    void handleSpiderTp(const SpiderTeleportData& tp);
    cocos2d::CCPoint getLastPosition();
    float getLastRotation();

private:
    friend class RemotePlayer;

    RemotePlayer* m_remotePlayer = nullptr;
    cocos2d::ccColor3B m_color1{}, m_color2{};
    bool m_teamInitialized = false;
    geode::Ref<NameLabel> m_nameLabel;
    geode::Ref<PlayerStatusIcons> m_statusIcons;

#ifdef GLOBED_DEBUG_INTERPOLATION
    geode::Ref<cocos2d::CCDrawNode> m_playerTrajectory = nullptr;
#endif

    bool m_isSecond = false;
    bool m_isPlatformer = false;
    bool m_isEditor = false;
    bool m_forceHideName = false;

    PlayerIconType m_prevMode = PlayerIconType::Unknown;

    // these 3 are used in robot and spider animations
    bool m_prevGrounded = false;
    bool m_prevStationary = false;
    bool m_prevFalling = false;

    // used in many animations
    bool m_prevNearby = false;

    // used in swing animations
    bool m_prevUpsideDown = false;

    // used to call onEnter, onExit
    bool m_prevPaused = false;

    bool m_prevRotating = false;

    cocos2d::CCPoint m_prevPosition{};
    float m_prevRotation = 0.f;

    //
    float m_tpColorDelta = 0.f;

    bool init(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond);
    PlayerIconData& icons();
    PlayerDisplayData& displayData();
    void updateOpacity();
    void updateIconType(PlayerIconType iconType);
    void updateRobotAnimation();
    void updateSpiderAnimation();
    void animateSwingFire(bool goingDown);
    void animateRobotFire(bool enable);
    void hideRobotFire();
    void showRobotFire();

    void updatePlayerObjectIcons(bool skipFrames);
    bool isPlayerNearby(const PlayerObjectData& data, const GameCameraState& camState);
    void spiderTeleportUpdateColor();
};

}
