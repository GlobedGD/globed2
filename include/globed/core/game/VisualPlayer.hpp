#pragma once

#include <Geode/Geode.hpp>
#include "PlayerState.hpp"

namespace globed {

class RemotePlayer;

class VisualPlayer : public PlayerObject {
public:
    static VisualPlayer* create(GJBaseGameLayer* gameLayer, bool isSecond);
    void updateFromData(const PlayerObjectData& data, const PlayerState& state);
    void cleanupObjectLayer();

private:
    friend class RemotePlayer;

    RemotePlayer* m_remotePlayer = nullptr;
    geode::Ref<cocos2d::CCDrawNode> m_playerTrajectory = nullptr;

    bool m_isSecond = false;
    bool m_isPlatformer = false;
    bool m_isEditor = false;

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

    bool init(GJBaseGameLayer* gameLayer, bool isSecond);
    void updateOpacity();
    void updateIconType(PlayerIconType iconType);
    void updateRobotAnimation();
    void updateSpiderAnimation();
    void animateSwingFire(bool goingDown);
    void animateRobotFire(bool enable);
    void hideRobotFire();
    void showRobotFire();
};

}
