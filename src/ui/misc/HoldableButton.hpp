#pragma once

#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <std23/move_only_function.h>

namespace globed {

/// Represents a button that functions similar to a regular CCMenuItemSpriteExtra,
/// but has additional functionality for when it's held down for a longer period of time.
class HoldableButton : public cocos2d::CCMenuItemSprite {
public:
    static HoldableButton* create(
        cocos2d::CCSprite* sprite,
        std23::move_only_function<void(HoldableButton*)> onClick,
        std23::move_only_function<void(HoldableButton*)> onHold
    );

    static HoldableButton* create(
        const char* spriteName,
        std23::move_only_function<void(HoldableButton*)> onClick,
        std23::move_only_function<void(HoldableButton*)> onHold
    ) {
        return create(
            cocos2d::CCSprite::create(spriteName),
            std::move(onClick),
            std::move(onHold)
        );
    }

    static HoldableButton* createWithSpriteFrameName(
        const char* spriteName,
        std23::move_only_function<void(HoldableButton*)> onClick,
        std23::move_only_function<void(HoldableButton*)> onHold
    ) {
        return create(
            cocos2d::CCSprite::createWithSpriteFrameName(spriteName),
            std::move(onClick),
            std::move(onHold)
        );
    }

    void selected() override;
    void unselected() override;
    void activate() override;

    /// Set the threshold for how long the button has to be held until it's considered a hold action.
    /// By default is 0.5 seconds. Set to 0 to disable hold actions.
    void setHoldThreshold(float seconds);

    /// Set the scale multiplier when button is selected
    void setScaleMult(float mult);

private:
    std23::move_only_function<void(HoldableButton*)> m_onClick;
    std23::move_only_function<void(HoldableButton*)> m_onHold;
    float m_holdThreshold = 0.5f;
    float m_scaleMult = 1.2f;
    bool m_shortHold = false;

    bool init(
        cocos2d::CCSprite* sprite,
        std23::move_only_function<void(HoldableButton*)> onClick,
        std23::move_only_function<void(HoldableButton*)> onHold
    );

    void rescaleTo(float s);
    void onHoldFinished(float s);
};

}