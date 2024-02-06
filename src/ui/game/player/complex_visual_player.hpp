#pragma once

#include "base_visual_player.hpp"
#include "status_icons.hpp"
#include <hooks/player_object.hpp>

class ComplexVisualPlayer : public cocos2d::CCNode, public BaseVisualPlayer {
public:
    bool init(RemotePlayer* parent, bool isSecond) override;
    void updateIcons(const PlayerIconData& icons) override;
    void updateData(const SpecificIconData& data, bool isDead, bool isPaused, bool isPracticing) override;
    void updateName() override;
    void updateIconType(PlayerIconType newType) override;
    void playDeathEffect() override;
    void playSpiderTeleport(const SpiderTeleportData& data) override;

    void updatePlayerObjectIcons();
    void toggleAllOff();
    void callToggleWith(PlayerIconType type, bool arg1, bool arg2);
    void callUpdateWith(PlayerIconType type, int icon);

    static ComplexVisualPlayer* create(RemotePlayer* parent, bool isSecond);

protected:
    PlayLayer* playLayer;
    ComplexPlayerObject* playerIcon;
    PlayerIconType playerIconType = PlayerIconType::Unknown;
    cocos2d::CCLabelBMFont* playerName;
    Ref<PlayerStatusIcons> statusIcons;


    // these 3 used in robot and spider anims
    bool wasGrounded = false;
    bool wasStationary = true;
    bool wasFalling = false;

    // used in swing anims
    bool wasUpsideDown = false;

    PlayerIconData storedIcons;

    static constexpr int ROBOT_FIRE_ACTION = 727;
    static constexpr int SWING_FIRE_ACTION = 728;

    void updateRobotAnimation();
    void updateSpiderAnimation();

    void animateRobotFire(bool enable);
    void onAnimateRobotFireIn();
    void onAnimateRobotFireOut();

    void animateSwingFire(bool goingDown);
};