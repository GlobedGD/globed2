#pragma once

#include <defs/geode.hpp>
#include <functional>
#include "progressbar.hpp"

class BetterSlider : public ProgressBar {
public:
    using Callback = std::function<void(BetterSlider*, double value)>;

    static BetterSlider* create();

    void setContentSize(const cocos2d::CCSize& size) override;

    void setLimits(double min, double max);

    void setCallback(Callback&& cb);
    void setCallback(const Callback& cb);

    // If `setLimits` was never called before, the return value is in range [0; 1], inclusive.
    // Otherwise, it is between `min` and `max` specified by the `setLimits` call.
    double getValue();

    // Return the slider value, range is [0; 1].
    double getValueRaw();

    // Set the slider value, should be in the range [0; 1], or [min; max] if `setLimits` has been called before.
    void setValue(double value);

    // Set the slider value, should be in the range [0; 1]
    void setValueRaw(double value);

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    // void ccTouchCancelled(cocos2d::CCTouch *touch, cocos2d::CCEvent* event) override {}
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

    // Slider knob (aka SliderThumb)
    class Knob : public cocos2d::CCNode {
    public:
        static Knob* create(BetterSlider* slider);
        void setState(bool held);
        bool isHeld();

    private:
        BetterSlider* slider;
        cocos2d::CCSprite *offSprite, *onSprite;

        bool init(BetterSlider* slider);
    };

private:
    double limMin = 0.0;
    double limMax = 1.0;
    Knob* knob = nullptr;
    Callback callback;
    float knobCorrection = 0.f;

    bool init() override;

    void setup(cocos2d::CCSize size);
};
