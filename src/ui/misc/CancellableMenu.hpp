#pragma once

#include <cocos2d.h>
#include <Geode/utils/function.hpp>

namespace globed {

/// This is a stupid class
class CancellableMenu : public cocos2d::CCMenu {
public:
    static CancellableMenu* create();

    void cancelTouch();

    /// Callback returns whether to swallow and consume this touch.
    void setTouchCallback(geode::Function<bool(bool within)> callback);

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    enum class State {
        None,
        Held,
        HeldPendingCancel,
        HeldCancelled
    };
    State m_state = State::None;
    geode::Function<bool(bool within)> m_touchCallback;
};

}