#pragma once
#include <defs.hpp>

class SliderWrapper : public cocos2d::CCNode {
public:
    static SliderWrapper* create(Slider* slider);

private:
    Slider* slider;

    bool init(Slider* slider);
};