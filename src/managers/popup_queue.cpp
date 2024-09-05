#include "popup_queue.hpp"

#include <util/misc.hpp>

using namespace geode::prelude;
// using util::misc::is_any_of_dynamic;

void PopupQueue::push(FLAlertLayer* popup, bool hideWhilePlaying) {
    (hideWhilePlaying ? queuedLowPrio : queuedHighPrio).push(popup);
}

void PopupQueue::update(float dt) {
    if (queuedLowPrio.empty() && queuedHighPrio.empty()) {
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

    // if there is a high priority popup, show it
    if (!queuedHighPrio.empty()) {
        auto popup = std::move(queuedHighPrio.front());
        queuedHighPrio.pop();
        popup->show();
        return;
    }

    // low prio popups,

    // if in a playlayer, dont show while unpaused
    if (typeinfo_cast<PlayLayer*>(layer)) {
        if (!getChildOfType<PauseLayer>(layer, 0)) {
            return;
        }
    }

    // noww show it
    auto popup = std::move(queuedLowPrio.front());
    queuedLowPrio.pop();
    popup->show();
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
