#include "slider.hpp"

#include <defs/assert.hpp>

using Knob = BetterSlider::Knob;
using namespace geode::prelude;

constexpr float FILL_PAD = 2.f;
constexpr float KNOB_PAD = 4.f;

bool BetterSlider::init() {
    if (!ProgressBar::init()) return false;

    Build<Knob>::create(this)
        .zOrder(10)
        .scale(0.6f)
        .id("knob")
        .anchorPoint(0.5f, 0.5f)
        .parent(this)
        .store(knob);

    // initialize
    this->setup(this->getContentSize());

    return true;
}

void BetterSlider::setContentSize(const CCSize& size) {
    if (!knob) {
        ProgressBar::setContentSize(size);
        return;
    }

    this->setup(size);
}

void BetterSlider::setup(CCSize size) {
    ProgressBar::setup(size);

    knob->setPositionY(outlineStart->getScaledContentHeight() / 2.f);

    // force update x pos
    this->setValueRaw(this->getValueRaw());
}

void BetterSlider::setLimits(double min, double max) {
    GLOBED_REQUIRE(min <= max, "min value bigger than max");

    this->limMin = min;
    this->limMax = max;
}

void BetterSlider::setCallback(Callback&& cb) {
    callback = std::move(cb);
}

void BetterSlider::setCallback(const Callback& cb) {
    callback = cb;
}

double BetterSlider::getValue() {
    double rawv = this->getValueRaw();

    return limMin + (limMax - limMin) * rawv;
}

double BetterSlider::getValueRaw() {
    return ProgressBar::getValue();
}

void BetterSlider::setValue(double value) {
    double diff = limMax - limMin;
    this->setValueRaw((value - limMin) / diff);
}

void BetterSlider::setValueRaw(double value) {
    ProgressBar::setValue(value);

    knob->setPositionX(KNOB_PAD + (this->getContentWidth() - KNOB_PAD * 2) * ProgressBar::getValue());
}

bool BetterSlider::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    CCMenu::ccTouchBegan(touch, event);

    // ok so
    auto relpos = this->convertTouchToNodeSpace(touch);
    auto knobSize = knob->getScaledContentSize();
    auto knobpos = knob->getPosition() - knobSize / 2.f;

    // check if it's inside of the rect
    if (
        relpos.x > knobpos.x
        && relpos.y > knobpos.y
        && relpos.x < (knobpos.x + knobSize.width)
        && relpos.y < (knobpos.y + knobSize.height)
    ) {
        knob->setState(true);
        float middle = knobpos.x + knobSize.width / 2.f;

        knobCorrection = -(relpos.x - middle);
        return true;
    }

    return false;
}

void BetterSlider::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    CCMenu::ccTouchEnded(touch, event);
    knob->setState(false);
}

void BetterSlider::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    CCMenu::ccTouchMoved(touch, event);

    // move the slider knob if it's selected
    if (knob->isHeld()) {
        auto relX = this->convertTouchToNodeSpace(touch).x - KNOB_PAD;
        float maxw = this->getContentSize().width - KNOB_PAD * 2;

        relX = std::clamp(relX + knobCorrection, 0.f, maxw);

        double rawFill = relX / maxw;

        this->setValueRaw(rawFill);

        if (callback) {
            callback(this, this->getValue());
        }
    }
}

BetterSlider* BetterSlider::create() {
    auto ret = new BetterSlider;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool Knob::init(BetterSlider* slider) {
    this->slider = slider;

    Build<CCSprite>::create("slider-thumb.png"_spr)
        .parent(this)
        .store(offSprite);

    Build<CCSprite>::create("slider-thumb-sel.png"_spr)
        .visible(false)
        .parent(this)
        .store(onSprite);

    auto sz = onSprite->getScaledContentSize();
    this->setScaledContentSize(sz);

    offSprite->setPosition(sz / 2.f);
    onSprite->setPosition(sz / 2.f);

    return true;
}

void Knob::setState(bool held) {
    offSprite->setVisible(!held);
    onSprite->setVisible(held);
}

bool Knob::isHeld() {
    return onSprite->isVisible();
}

Knob* Knob::create(BetterSlider* slider) {
    auto ret = new Knob;
    if (ret->init(slider)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
