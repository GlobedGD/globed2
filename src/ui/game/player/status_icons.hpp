#pragma once
#include <defs/all.hpp>

class PlayerStatusIcons : public cocos2d::CCNode {
public:
    void updateStatus(bool paused, bool practicing, bool speaking, float loudness, bool force = false);
    void updateLoudnessIcon(float dt);

    static PlayerStatusIcons* create();

private:
    enum class Loudness {
        Low, Medium, High
    };

    cocos2d::CCNode* iconWrapper = nullptr;
    bool wasPaused = false, wasPracticing = false, wasSpeaking = false;
    Loudness wasLoudness = Loudness::Low;
    float lastLoudness = 0.f;
    float nameScale = 0.f;

    bool init() override;

    static Loudness loudnessToCategory(float loudness);
};