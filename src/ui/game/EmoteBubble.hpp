#pragma once

#include <Geode/Geode.hpp>
#include <globed/prelude.hpp>

namespace globed {

class EmoteBubble : public CCNode {
public:
    static EmoteBubble* create();

    void playEmote(uint32_t emoteId);
    void setOpacity(uint8_t op);

    bool isPlaying();

private:
    CCSprite* m_emoteSpr = nullptr;
    CCSprite* m_bubbleSpr = nullptr;

    bool init();

    // Needed because VisualPlayer calls setVisible when culled
    void customToggleVis(bool vis);
};

}