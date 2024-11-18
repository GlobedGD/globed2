#include "loading_circle.hpp"

using namespace geode::prelude;

bool BetterLoadingCircle::init(bool spinByDefault) {
    if (!CCSprite::initWithFile("loadingCircle.png")) {
        return false;
    }

    this->runAction(CCRepeatForever::create(
        CCRotateBy::create(1.0f, 360.f)
    ));

    this->setBlendFunc({GL_SRC_ALPHA, GL_ONE});

    // if disabled, hide the circle
    if (!spinByDefault) {
        this->setOpacity(0);
        this->setVisible(false);
        this->onExit();
    }

    return true;
}

void BetterLoadingCircle::fadeOut() {
    this->runAction(
        CCSequence::create(
            CCFadeTo::create(0.25f, 0),
            CCCallFunc::create(this, callfunc_selector(BetterLoadingCircle::deactivate)),
            nullptr
        )
    );
}

void BetterLoadingCircle::deactivate() {
    this->setVisible(false);
    this->onExit();
}

void BetterLoadingCircle::fadeIn() {
    if (this->isVisible()) return;

    this->setVisible(true);
    this->onEnter();
    this->setOpacity(0);
    this->runAction(CCFadeTo::create(0.25f, 255));
}

BetterLoadingCircle* BetterLoadingCircle::create(bool spinByDefault) {
    auto ret = new BetterLoadingCircle;
    if (ret->init(spinByDefault)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
