#pragma once
#include <Geode/Geode.hpp>

#include <cue/PlayerIcon.hpp>

namespace globed {

class ProgressIcon : public cocos2d::CCNode {
public:
    static ProgressIcon* create();

    void updateIcons(const cue::Icons& data);
    void updatePosition(float xpos, bool practicing);
    void toggleLine(bool enabled);
    void togglePracticeSprite(bool enabled);
    void setForceOnTop(bool state);

private:
    cocos2d::CCLayerColor* m_line = nullptr;
    cue::PlayerIcon* m_icon = nullptr;
    bool m_forceOnTop = false;

    bool init();
    void recalcOpacity();
};

}