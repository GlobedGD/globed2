#pragma once

#include <globed/prelude.hpp>

namespace globed {

// Wrapper around FMODLevelVisualizer but horizontal (yay)
class AudioVisualizer : public CCNode {
public:
    static AudioVisualizer* create();

    void setVolume(float vol);
    void resetMaxVolume();

    void setScaleX(float scale) override;

private:
    FMODLevelVisualizer* m_vis;
    float m_maxVolume = 0.f;

    bool init() override;
};

}