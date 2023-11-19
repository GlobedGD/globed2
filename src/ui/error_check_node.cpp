#include "error_check_node.hpp"

#include <managers/error_queues.hpp>

using namespace geode::prelude;

bool ErrorCheckNode::init() {
    if (!CCNode::init()) return false;

    CCScheduler::get()->scheduleSelector(schedule_selector(ErrorCheckNode::updateErrors), this, 0.25f, false);
    return true;
}

void ErrorCheckNode::updateErrors(float _unused) {
    auto* currentScene = CCDirector::get()->getRunningScene();
    if (!currentScene || !currentScene->getChildren() || currentScene->getChildrenCount() == 0) return;

    auto* currentLayer = currentScene->getChildren()->objectAtIndex(0);
    
    // do nothing during transitions or loading
    if (typeinfo_cast<CCTransitionScene*>(currentScene) || typeinfo_cast<LoadingLayer*>(currentLayer)) {
        return;
    }

    // TODO maybe dont show errors in playlayer if we aren't paused?

    auto errors = ErrorQueues::get().getErrors();
    auto warnings = ErrorQueues::get().getWarnings();
    auto notices = ErrorQueues::get().getNotices();

    for (auto& warn : warnings) {
        Notification::create(warn, NotificationIcon::Warning, 2.5f)->show();
    }

    if (errors.size() > 2) {
        errors.resize(2);
    }

    for (auto& error : errors) {
        FLAlertLayer::create("Globed error", error, "Ok")->show();
    }

    if (notices.size() > 2) {
        notices.resize(2);
    }

    for (auto& notice : notices) {
        FLAlertLayer::create("Globed notice", notice, "Ok")->show();
    }
}