#pragma once

#include "base_visual_player.hpp"
#include "status_icons.hpp"
#include <hooks/player_object.hpp>

class ComplexVisualPlayer : public cocos2d::CCNode, public BaseVisualPlayer {
public:
    bool init(RemotePlayer* parent, bool isSecond) override;
    void updateIcons(const PlayerIconData& icons) override;
    void updateData(
        const SpecificIconData& data,
        const VisualPlayerState& playerData,
        bool isSpeaking,
        float loudness
    ) override;
    void updateName() override;
    void updateIconType(PlayerIconType newType) override;
    void playDeathEffect() override;
    void playSpiderTeleport(const SpiderTeleportData& data) override;
    void playJump() override;
    cocos2d::CCPoint getPlayerPosition() override;

    void updatePlayerObjectIcons(bool skipFrames = false);
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
    bool isPlatformer;

    // these 3 used in robot and spider anims
    bool wasGrounded = false;
    bool wasStationary = true;
    bool wasFalling = false;

    // used in spider anims
    cocos2d::ccColor3B storedMainColor, storedSecondaryColor;
    float tpColorDelta = 0.f;

    // used in swing anims
    bool wasUpsideDown = false;

    PlayerIconData storedIcons;

    // used for async icon loading
    struct AsyncLoadRequest {
        int key;
        int iconId;
        PlayerIconType iconType;
    };

    int iconsLoaded = 0;
    std::unordered_map<int, AsyncLoadRequest> asyncLoadRequests;

    static constexpr int ROBOT_FIRE_ACTION = 1000727;
    static constexpr int SWING_FIRE_ACTION = 1000728;
    static constexpr int SPIDER_TELEPORT_COLOR_ACTION = 1000729;

    void spiderTeleportUpdateColor();

    void updateRobotAnimation();
    void updateSpiderAnimation();

    void animateRobotFire(bool enable);
    void onAnimateRobotFireIn();
    void onAnimateRobotFireOut();

    void animateSwingFire(bool goingDown);
    void updateOpacity();

    void tryLoadIconsAsync();
    void onFinishedLoadingIconAsync();
    // fucking hell i hate this
    void asyncIconLoadedIntermediary(cocos2d::CCObject*);
};