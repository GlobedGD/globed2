#pragma once
#include <Geode/Geode.hpp>

#include <cue/PlayerIcon.hpp>

namespace globed {

struct GameCameraState;

class ProgressArrow : public cocos2d::CCNode {
public:
    static ProgressArrow* create();

    void updateIcons(const cue::Icons& data);
    void updatePosition(const GameCameraState& cam, cocos2d::CCPoint playerPos, double angle);

private:
    cue::PlayerIcon* m_icon = nullptr;

    bool init();
};

}