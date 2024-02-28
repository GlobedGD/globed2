#pragma once
#include <defs/all.hpp>

// Wrapper around FMODLevelVisualizer but horizontal (yay)
class GlobedAudioVisualizer : public cocos2d::CCNode {
public:
    static GlobedAudioVisualizer* create();

    void setVolume(float vol);
    // same as setVolume but without multiplying by a certain value
    void setVolumeRaw(float vol);

    void resetMaxVolume();

    void setScaleX(float scale) override;

    FMODLevelVisualizer* visNode;
    float maxVolume = 0.f;

private:
    bool init() override;

};
