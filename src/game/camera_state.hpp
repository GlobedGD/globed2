#pragma once

struct GameCameraState {
    cocos2d::CCPoint cameraOrigin;
    cocos2d::CCPoint visibleOrigin;
    cocos2d::CCSize visibleCoverage;
    float zoom;

    cocos2d::CCSize cameraCoverage() const {
        return visibleCoverage / std::fabs(zoom);
    }
};
