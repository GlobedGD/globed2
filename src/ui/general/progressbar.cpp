#include "progressbar.hpp"

#include <defs/assert.hpp>

using namespace geode::prelude;

constexpr static float FILL_PAD = 2.f;

// this is kinda bad but eh whatever
constexpr static float OUTLINE_SCALE = 0.69f;

bool ProgressBar::init() {
    if (!CCMenu::init()) return false;

    // stupid ass cocos conventions why do i have to do that
    this->ignoreAnchorPointForPosition(false);

    Build<CCSprite>::create("slider-start.png"_spr)
        .scale(OUTLINE_SCALE)
        .zOrder(5)
        .anchorPoint(0.f, 0.5f)
        .id("outline-start")
        .parent(this)
        .store(outlineStart);

    Build<CCSprite>::create("slider-end.png"_spr)
        .scale(OUTLINE_SCALE)
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
        .scale(OUTLINE_SCALE)
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
    this->setValue(1.0);

    return true;
}

void ProgressBar::setContentSize(const CCSize& size) {
    if (!outlineStart) {
        CCMenu::setContentSize(size);
        return;
    }

    this->setup(size);
}

void ProgressBar::setup(CCSize size) {
    float width = size.width;
    float height = outlineStart->getScaledContentHeight();

    float outlineMiddleWidth = std::max(0.f, width - outlineStart->getScaledContentWidth() - outlineEnd->getScaledContentWidth());

    outlineMiddle->setTextureRect({0.f, 0.f, outlineMiddleWidth / OUTLINE_SCALE, height / OUTLINE_SCALE});

    outlineStart->setPosition({0.f, height / 2.f});
    outlineEnd->setPosition({width, height / 2.f});
    outlineMiddle->setPosition({width / 2.f, height / 2.f});
    fill->setPosition({FILL_PAD, height / 2.f});

    CCMenu::setContentSize({width, height});
}

double ProgressBar::getValue() {
    return rawvalue;
}

void ProgressBar::setValue(double value) {
    this->rawvalue = std::clamp(value, 0.0, 1.0);

    // update fill
    float maxWidth = this->getContentWidth() - FILL_PAD;
    float range = maxWidth - FILL_PAD;

    float width = range * rawvalue;
    fill->setTextureRect({0.f, 0.f, width, outlineStart->getScaledContentHeight()});
}

ProgressBar* ProgressBar::create() {
    auto ret = new ProgressBar;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
