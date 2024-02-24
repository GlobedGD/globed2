#pragma once
#include <Geode/Geode.hpp>

class GlobedVoiceOverlay : public cocos2d::CCNode {
public:
    static GlobedVoiceOverlay* create();

    void updateOverlay();
    void updateOverlaySoft();
    void addPlayer(int accountId);

private:
    bool init() override;
};
