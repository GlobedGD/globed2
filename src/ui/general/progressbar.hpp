#pragma once

#include <defs/geode.hpp>

class ProgressBar : public cocos2d::CCMenu {
public:
    static ProgressBar* create();

    void setContentSize(const cocos2d::CCSize& size) override;

    // Value is returned in range [0; 1]
    double getValue();

    // Set the slider value, should be in the range [0; 1]
    void setValue(double value);

protected:
    double rawvalue = 0.0;

    cocos2d::CCSprite
        *outlineStart = nullptr,
        *outlineMiddle = nullptr,
        *outlineEnd = nullptr;

    cocos2d::CCSprite* fill = nullptr;

    bool init() override;

    void setup(cocos2d::CCSize size);
};
