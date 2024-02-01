#include "error_check_node.hpp"

#include <hooks/play_layer.hpp>
#include <managers/error_queues.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;

bool ErrorCheckNode::init() {
    if (!CCNode::init()) return false;

    CCScheduler::get()->scheduleSelector(schedule_selector(ErrorCheckNode::updateErrors), this, 0.2f, false);
    return true;
}

void ErrorCheckNode::updateErrors(float) {
    util::debugging::DataWatcher::get().updateAll();

    auto* currentScene = CCScene::get();
    if (!currentScene || !currentScene->getChildren() || currentScene->getChildrenCount() == 0) return;

    auto* currentLayer = currentScene->getChildren()->objectAtIndex(0);

    // do nothing during transitions or loading

    if (typeinfo_cast<CCTransitionScene*>(currentScene) || typeinfo_cast<LoadingLayer*>(currentLayer)) {
        return;
    }

    std::vector<std::string> warnings, successes;

    try {
        warnings = ErrorQueues::get().getWarnings();
        successes = ErrorQueues::get().getSuccesses();
    } catch (const std::system_error& e) {
        // sometimes, when exiting the game, macos will do weird stuff and throw an exception trying to lock a mutex.
        // we want to prevent that.
#ifndef GEODE_IS_MACOS
        log::warn("failed to pop the warnings: {}", e.what());
#endif
        return;
    }

    for (auto& warn : warnings) {
        Notification::create(warn, NotificationIcon::Warning, 2.5f)->show();
    }

    for (auto& success : successes) {
        Notification::create(success, NotificationIcon::Success, 2.5f)->show();
    }

    // if we are in PlayLayer, don't show errors unless paused

    auto playlayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (playlayer != nullptr && !playlayer->isPaused()) {
        return;
    }

    auto errors = ErrorQueues::get().getErrors();
    auto notices = ErrorQueues::get().getNotices();

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

ErrorCheckNode* ErrorCheckNode::create() {
    auto* ret = new ErrorCheckNode;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}