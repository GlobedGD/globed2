#include "HoldableButton.hpp"
#include "CancellableMenu.hpp"

using namespace geode::prelude;

namespace globed {

constexpr int ACTION_TAG = 83487329;

bool HoldableButton::init(
    cocos2d::CCSprite* sprite,
    std23::move_only_function<void(HoldableButton*)> onClick,
    std23::move_only_function<void(HoldableButton*)> onHold
) {
    if (!CCMenuItemSprite::initWithNormalSprite(sprite, nullptr, nullptr, this, nullptr)) {
        return false;
    }

    this->setContentSize(sprite->getScaledContentSize());

    m_onClick = std::move(onClick);
    m_onHold = std::move(onHold);

    return true;
}

void HoldableButton::selected() {
    if (!m_bEnabled) return;
    CCMenuItemSprite::selected();

    m_shortHold = true;
    this->rescaleTo(m_scaleMult);

    if (m_holdThreshold > 0.f) {
        this->scheduleOnce(schedule_selector(HoldableButton::onHoldFinished), m_holdThreshold);
    }
}

void HoldableButton::setScaleMult(float mult) {
    m_scaleMult = mult;
}

void HoldableButton::unselected() {
    if (!m_bEnabled) return;
    CCMenuItemSprite::unselected();

    this->rescaleTo(1.0f);
    this->unschedule(schedule_selector(HoldableButton::onHoldFinished));
}

void HoldableButton::rescaleTo(float s) {
    this->stopActionByTag(ACTION_TAG);
    auto action = CCEaseBounceOut::create(
        CCScaleTo::create(0.3f, s)
    );
    action->setTag(ACTION_TAG);
    this->runAction(action);
}

void HoldableButton::activate() {
    if (!m_bEnabled || !m_shortHold) return;

    CCMenuItemSprite::activate();
    m_shortHold = false;

    // no easing
    this->stopActionByTag(ACTION_TAG);
    this->setScale(1.0f);
    this->unschedule(schedule_selector(HoldableButton::onHoldFinished));

    if (m_onClick) {
        m_onClick(this);
    }
}

void HoldableButton::onHoldFinished(float s) {
    m_shortHold = false;

    if (m_onHold) {
        m_onHold(this);
    }

    if (auto menu = typeinfo_cast<CancellableMenu*>(this->getParent())) {
        menu->cancelTouch(); // should call unselected()
    }
}

void HoldableButton::setHoldThreshold(float seconds) {
    m_holdThreshold = seconds;
}

HoldableButton* HoldableButton::create(
    CCSprite* sprite,
    std23::move_only_function<void(HoldableButton*)> onClick,
    std23::move_only_function<void(HoldableButton*)> onHold
) {
    auto ret = new HoldableButton();
    if (ret->init(sprite, std::move(onClick), std::move(onHold))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}