#pragma once
#include <defs/all.hpp>

#include <data/types/gd.hpp>
#include <game/camera_state.hpp>
#include <ui/general/simple_player.hpp>

class PlayerProgressArrow : public cocos2d::CCNode {
public:
    static PlayerProgressArrow* create();

    // cameraOrigin is the bottom-left corner of the screen in objectLayer
    // visibleOrigin and visibleCoverage is just 0,0 and getWinSize()
    void updatePosition(
        const GameCameraState& camState,
        cocos2d::CCPoint playerPosition
    );

    void updateIcons(const PlayerIconData& data);

private:
    GlobedSimplePlayer* playerIcon = nullptr;

    bool init() override;
};
