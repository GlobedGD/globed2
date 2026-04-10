#pragma once

#include <Geode/Geode.hpp>
#include <globed/prelude.hpp>

namespace globed {

class EmoteBubble : public cocos2d::CCNodeRGBA {
public:
    static EmoteBubble* create();

    void playEmote(uint32_t emoteId, std::shared_ptr<RemotePlayer> player);
    void setOpacity(uint8_t op) override;
    uint8_t getOpacity() override;
    void setOpacityMult(float mult);
    void flipBubble(bool flipped);
    void update(float dt) override;

    bool isPlaying();

private:
    CCSprite* m_emoteSpr = nullptr;
    CCSprite* m_bubbleSpr = nullptr;
    float m_initialScale;
    float m_opacityMult = 1.f;
    uint8_t m_baseOpacity = 255;

    bool init() override;

    // Needed because VisualPlayer calls setVisible when culled
    void customToggleVis(bool vis);
};

}