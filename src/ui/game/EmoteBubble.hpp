#pragma once

#include <Geode/Geode.hpp>
#include <globed/prelude.hpp>

namespace globed {

class EmoteBubble : public CCNode {
public:
    static EmoteBubble *create();

    void playEmote(uint32_t emoteId, std::shared_ptr<RemotePlayer> player);
    void setOpacity(uint8_t op);
    void flipBubble(bool flipped);
    void update(float dt) override;

    bool isPlaying();

private:
    CCSprite *m_emoteSpr = nullptr;
    CCSprite *m_bubbleSpr = nullptr;
    float m_initialScale;

    bool init() override;

    // Needed because VisualPlayer calls setVisible when culled
    void customToggleVis(bool vis);
};

} // namespace globed