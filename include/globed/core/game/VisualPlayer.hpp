#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/core/game/PlayerStatusIcons.hpp>
#include <Geode/Geode.hpp>

namespace globed {

static constexpr int ROBOT_FIRE_ACTION = 1000727;
static constexpr int SWING_FIRE_ACTION = 1000728;
static constexpr int SPIDER_TELEPORT_COLOR_ACTION = 1000729;
static constexpr int SPIDER_DASH_CIRCLE_WAVE_TAG = 234562345;
static constexpr int SPIDER_DASH_SPRITE_TAG = 234562347;
static constexpr int DEATH_EFFECT_TAG = 234562349;
static constexpr float NAME_OFFSET = 30.f;
static constexpr float STATUS_ICONS_OFFSET = NAME_OFFSET + 15.f;

class RemotePlayer;
class NameLabel;
class EmoteBubble;
struct GameCameraState;

class GLOBED_DLL VisualPlayer : public PlayerObject {
public:
    VisualPlayer();
    GLOBED_NOCOPY(VisualPlayer);
    GLOBED_NOMOVE(VisualPlayer);

    static VisualPlayer* create(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond, bool localPlayer);
    void updateFromData(const PlayerObjectData& data, const PlayerState& state, const GameCameraState& camState, bool forceHide);
    void cleanupObjectLayer();

    PlayerIconData& icons();
    PlayerDisplayData& displayData();

    void setStickyState(bool p1, bool sticky);

    void updateDisplayData();
    void updateTeam(uint16_t teamId);
    void playDeathEffect();
    void handleSpiderTp(const SpiderTeleportData& tp);
    CCPoint getLastPosition();
    float getLastRotation();
    void playPlatformerJump();

    void playEmote(uint32_t emoteId);

    void setVisible(bool vis) override;

private:
    friend class RemotePlayer;

    RemotePlayer* m_remotePlayer = nullptr;
    ccColor3B m_color1{}, m_color2{};
    bool m_teamInitialized = false;
    NameLabel* m_nameLabel;
    Ref<PlayerStatusIcons> m_statusIcons;
    Ref<cocos2d::CCDrawNode> m_playerTrajectory;
    EmoteBubble* m_emoteBubble = nullptr;

    bool m_isLocalPlayer = false;
    bool m_isSecond = false;
    bool m_isEditor = false;
    bool m_forceHideName = false;
    bool m_playingDeathEffect = false;

    PlayerIconType m_prevMode = PlayerIconType::Unknown;

    // these 3 are used in robot and spider animations
    bool m_prevGrounded = false;
    bool m_prevStationary = false;
    bool m_prevFalling = false;

    // used in many animations
    bool m_prevNearby = false;

    // used in swing animations and updating for gravity
    bool m_prevUpsideDown = false;

    // used for platformer jump anim
    bool m_didPlatformerJump = false;

    // used to call onEnter, onExit
    bool m_prevPaused = false;

    bool m_prevRotating = false;

    CCPoint m_prevPosition{};
    float m_prevRotation = 0.f;

    //
    float m_tpColorDelta = 0.f;

    // collision
    bool m_p1Sticky = false;
    bool m_p2Sticky = false;

    bool init(GJBaseGameLayer* gameLayer, RemotePlayer* rp, CCNode* playerNode, bool isSecond, bool localPlayer);
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
    void cancelPlatformerJumpAnim();

    void updateLerpTrajectory(const PlayerObjectData& data);
};

}
