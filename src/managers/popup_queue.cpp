#include "popup_queue.hpp"

#include <hooks/flalertlayer.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;
// using util::misc::is_any_of_dynamic;

void PopupQueue::pushNoDelay(geode::Ref<FLAlertLayer> popup, bool hideWhilePlaying) {
    (hideWhilePlaying ? queuedLowPrio : queuedHighPrio).push(std::move(popup));
}

void PopupQueue::push(FLAlertLayer* popup, CCNode* invoker, bool hideWhilePlaying) {
    if (!invoker) {
        auto scene = CCScene::get();
        if (scene && scene->getChildrenCount() > 0) {
            invoker = static_cast<CCNode*>(scene->getChildren()->objectAtIndex(0));
        }
    }

    if (!invoker) {
        log::warn("PopupQueue internal error: invoker is null and failed to find scene child. Ignoring popup.");
        return;
    }

    delayedPopups.push_back(DelayedPopup {
        .popup = Ref(popup),
        .invokerLayer = invoker,
        .lowPrioQueue = hideWhilePlaying
    });
}

void PopupQueue::update(float dt) {
    bool hasQueuedPopups = !queuedLowPrio.empty() || !queuedHighPrio.empty();

    if (!hasQueuedPopups && delayedPopups.empty()) {
        return;
    }

    auto scene = CCScene::get();
    if (!scene || scene->getChildrenCount() == 0) return;

    // check if we are transitioning 🏳️‍⚧️
    if (typeinfo_cast<CCTransitionScene*>(scene)) {
        return;
    }

    // dont show on some layers
    auto layer = static_cast<CCNode*>(scene->getChildren()->objectAtIndex(0));
    if (typeinfo_cast<LoadingLayer*>(layer)) {
        return;
    }

    // check if any of the delayed popups can be pushed now
    for (auto it = delayedPopups.begin(); it != delayedPopups.end();) {
        if (it->invokerLayer == layer) {
            ++it;
        } else {
            // layer changed, we can push it to the queue now
            this->pushNoDelay(std::move(it->popup), it->lowPrioQueue);
            hasQueuedPopups = true;
            delayedPopups.erase(it);

            // dont increment iterator
        }
    }

    if (!hasQueuedPopups) {
        return;
    }

    // if there is a high priority popup, show it
    if (!queuedHighPrio.empty()) {
        auto popup = std::move(queuedHighPrio.front());
        queuedHighPrio.pop();
        SceneManager::get()->keepAcrossScenes(popup);
        // popup->show();
        return;
    }

    // low prio popups,

    // if in a playlayer, dont show while unpaused
    if (typeinfo_cast<PlayLayer*>(layer)) {
        if (!layer->getChildByType<PauseLayer>(0)) {
            return;
        }
    }

    // noww show it
    auto popup = std::move(queuedLowPrio.front());
    queuedLowPrio.pop();
    SceneManager::get()->keepAcrossScenes(popup);
    // popup->show();
}

PopupQueue::PopupQueue() {}

PopupQueue* PopupQueue::get() {
    static auto instance = []{
        auto ret = new PopupQueue;
        ret->scheduleUpdate();
        ret->onEnter();
        return ret;
    }();

    return instance;
}
