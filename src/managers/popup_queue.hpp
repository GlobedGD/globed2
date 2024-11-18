#pragma once

#include <util/singleton.hpp>
#include <cocos2d.h>
#include <Geode/binding/FLAlertLayer.hpp>
#include <queue>

// not thread safe
class PopupQueue : public cocos2d::CCNode {
public:
    static PopupQueue* get();

    void pushNoDelay(geode::Ref<FLAlertLayer> popup, bool hideWhilePlaying = true);

    // call this when in an init hook for example
    void push(FLAlertLayer* popup, cocos2d::CCNode* invokerLayer = nullptr, bool hideWhilePlaying = true);

private:
    struct DelayedPopup {
        geode::Ref<FLAlertLayer> popup;
        cocos2d::CCNode* invokerLayer;
        bool lowPrioQueue;
    };

    std::queue<geode::Ref<FLAlertLayer>> queuedHighPrio, queuedLowPrio;
    std::vector<DelayedPopup> delayedPopups;

    PopupQueue();

    void update(float dt) override;
};
