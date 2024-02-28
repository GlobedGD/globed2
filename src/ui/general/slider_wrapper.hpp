#pragma once
#include <defs/all.hpp>

class SliderWrapper : public cocos2d::CCNode {
public:
    static SliderWrapper* create(Slider* slider);

    Slider* slider;

private:
    bool init(Slider* slider);
};