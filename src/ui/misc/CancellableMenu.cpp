#include "CancellableMenu.hpp"

using namespace geode::prelude;

namespace globed {

void CancellableMenu::cancelTouch()
{
    m_state = State::HeldPendingCancel;
}

void CancellableMenu::setTouchCallback(std23::move_only_function<bool(bool within)> callback)
{
    m_touchCallback = std::move(callback);
}

bool CancellableMenu::ccTouchBegan(CCTouch *touch, CCEvent *event)
{
    auto tpos = this->convertTouchToNodeSpace(touch);
    bool within = CCRect{CCPoint{}, this->getContentSize()}.containsPoint(tpos);

    bool ret = CCMenu::ccTouchBegan(touch, event);
    if (m_touchCallback) {
        bool shouldConsume = m_touchCallback(within);
        if (shouldConsume) {
            CCMenu::ccTouchCancelled(touch, event);
            m_pSelectedItem = nullptr;
            m_state = State::None;
            return true;
        }
    }

    m_state = ret ? State::Held : State::None;

    return ret;
}

void CancellableMenu::ccTouchEnded(CCTouch *touch, CCEvent *event)
{
    m_state = State::None;
    CCMenu::ccTouchEnded(touch, event);
}

void CancellableMenu::ccTouchCancelled(CCTouch *touch, CCEvent *event)
{
    m_state = State::None;
    CCMenu::ccTouchCancelled(touch, event);
}

void CancellableMenu::ccTouchMoved(CCTouch *touch, CCEvent *event)
{
    switch (m_state) {
    case State::None:
        break;
    case State::Held:
        CCMenu::ccTouchMoved(touch, event);
        break;
    case State::HeldPendingCancel: {
        m_state = State::HeldCancelled;
        this->ccTouchCancelled(touch, event);
        break;
    };
    case State::HeldCancelled:
        break;
    }
}

CancellableMenu *CancellableMenu::create()
{
    auto ret = new CancellableMenu();
    ret->init();
    ret->autorelease();
    return ret;
}

} // namespace globed