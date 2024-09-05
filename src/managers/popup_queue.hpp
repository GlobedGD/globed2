#pragma once

#include <util/singleton.hpp>
#include <cocos2d.h>
#include <Geode/binding/FLAlertLayer.hpp>
#include <queue>

class PopupQueue : public cocos2d::CCNode {
public:
    static PopupQueue* get();

    void push(FLAlertLayer* popup, bool hideWhilePlaying = true);

private:
    std::queue<geode::Ref<FLAlertLayer>> queuedHighPrio, queuedLowPrio;

    PopupQueue();

    void update(float dt) override;
};
