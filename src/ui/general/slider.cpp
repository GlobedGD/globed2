#include "slider.hpp"

#include <defs/assert.hpp>

using Knob = BetterSlider::Knob;
using namespace geode::prelude;

constexpr float FILL_PAD = 2.f;

bool BetterSlider::init() {
    if (!CCMenu::init()) return false;

    // stupid ass cocos conventions why do i have to do that
    this->ignoreAnchorPointForPosition(false);

    Build<Knob>::create(this)
        .zOrder(10)
        .scale(0.6f)
        .id("knob")
        .anchorPoint(0.5f, 0.5f)
        .parent(this)
        .store(knob);

    Build<CCSprite>::create("slider-start.png"_spr)
        .zOrder(5)
        .anchorPoint(0.f, 0.5f)
        .id("outline-start")
        .parent(this)
        .store(outlineStart);

    Build<CCSprite>::create("slider-end.png"_spr)
        .zOrder(5)
        .anchorPoint(1.f, 0.5f)
        .id("outline-end")
        .parent(this)
        .store(outlineEnd);

    ccTexParams tp = {
        GL_LINEAR, GL_LINEAR,
        GL_REPEAT, GL_CLAMP_TO_EDGE
    };

    Build<CCSprite>::create("slider-middle.png"_spr)
        .zOrder(4)
        .anchorPoint(0.5f, 0.5f)
        .id("outline-middle")
        .with([&](CCSprite* sprite) {
            sprite->getTexture()->setTexParameters(&tp);
        })
        .parent(this)
        .store(outlineMiddle);

    Build<CCSprite>::create("slider-fill.png"_spr)
        .scaleY(0.7f)
        .zOrder(1)
        .anchorPoint(0.0f, 0.5f)
        .id("fill")
        .with([&](CCSprite* sprite) {
            sprite->getTexture()->setTexParameters(&tp);
        })
        .parent(this)
        .store(fill);

    // initialize
    this->setContentSize({64.f, 0.f});

    return true;
}

void BetterSlider::setContentSize(const CCSize& size) {
    if (!knob) {
        CCMenu::setContentSize(size);
        return;
    }

    this->setup(size);
}

void BetterSlider::setup(CCSize size) {
    float width = size.width;
    float height = outlineStart->getContentHeight();

    float outlineMiddleWidth = std::max(0.f, width - outlineStart->getContentWidth() - outlineEnd->getContentWidth());

    outlineMiddle->setTextureRect({0.f, 0.f, outlineMiddleWidth, height});

    outlineStart->setPosition({0.f, height / 2.f});
    outlineEnd->setPosition({width, height / 2.f});
    outlineMiddle->setPosition({width / 2.f, height / 2.f});
    fill->setPosition({FILL_PAD, height / 2.f});

    knob->setPositionY(height / 2.f);

    CCMenu::setContentSize({width, height});
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
    return rawvalue;
}

void BetterSlider::setValue(double value) {
    double diff = limMax - limMin;
    this->setValueRaw((value - limMin) / diff);
}

void BetterSlider::setValueRaw(double value) {
    this->rawvalue = std::clamp(value, 0.0, 1.0);
    knob->setPositionX(this->getContentWidth() * value);

    // update fill
    float maxWidth = this->getContentWidth() - FILL_PAD;
    float range = maxWidth - FILL_PAD;

    float width = range * rawvalue;
    fill->setTextureRect({0.f, 0.f, width, outlineStart->getContentHeight()});
}

bool BetterSlider::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    CCMenu::ccTouchBegan(touch, event);

    // ok so
    auto relpos = this->convertTouchToNodeSpace(touch);
    const auto& knobSize = knob->getScaledContentSize();
    const auto& knobpos = knob->getPosition() - knobSize / 2.f;

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
        auto relX = this->convertTouchToNodeSpace(touch).x;
        float maxw = this->getContentSize().width;

        relX = std::clamp(relX + knobCorrection, 0.f, maxw);

        knob->setPositionX(relX);

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
