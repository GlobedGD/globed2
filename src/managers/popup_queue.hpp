#pragma once

#include <util/singleton.hpp>
#include <cocos2d.h>
#include <Geode/binding/FLAlertLayer.hpp>
#include <queue>

// not thread safe
class PopupQueue : public cocos2d::CCNode {
public:
    static PopupQueue* get();

    // This will show the popup as soon as it is appropriate to do so, for example it will wait if the user is currently in a transition.
    // NOTE: the popup must call SceneManager::forget(this) in onClose, otherwise this will be buggy!
    void pushNoDelay(geode::Ref<FLAlertLayer> popup, bool hideWhilePlaying = true);

    // This will show the popup as soon as the user leaves the `invokerLayer` (which defaults to the current scene).
    // Call this when in an init hook for example.
    // NOTE: the popup must call SceneManager::forget(this) in onClose, otherwise this will be buggy!
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
