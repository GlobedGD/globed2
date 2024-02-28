#include "error_check_node.hpp"

#include <hooks/play_layer.hpp>
#include <hooks/flalertlayer.hpp>
#include <managers/error_queues.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

constexpr auto BLOCK_CLOSING_FOR = util::time::millis(375);

bool ErrorCheckNode::init() {
    if (!CCNode::init()) return false;

    CCScheduler::get()->scheduleSelector(schedule_selector(ErrorCheckNode::updateErrors), this, 0.1f, false);
    return true;
}

void ErrorCheckNode::updateErrors(float) {
    util::debug::DataWatcher::get().updateAll();

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
        log::warn("failed to pop the warnings: {}", e.what());
        return;
    }

    for (auto& warn : warnings) {
        Notification::create(warn, NotificationIcon::Warning, 2.0f)->show();
    }

    for (auto& success : successes) {
        Notification::create(success, NotificationIcon::Success, 1.25f)->show();
    }

    // if we are in PlayLayer, don't show errors unless paused

    auto playlayer = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (playlayer && !playlayer->isPaused()) {
        return;
    }

    auto errors = ErrorQueues::get().getErrors();
    auto notices = ErrorQueues::get().getNotices();

    for (auto& error : errors) {
        if (canShowFLAlert()) {
            auto alert = static_cast<HookedFLAlertLayer*>(FLAlertLayer::create("Globed error", error, "Ok"));
            alert->setID("error-popup"_spr);
            alert->blockClosingFor(BLOCK_CLOSING_FOR);
            alert->show();
        } else {
            log::warn("cant show flalert, ignoring error: {}", error);
        }
    }

    for (auto& notice : notices) {
        if (canShowFLAlert()) {
            auto alert = static_cast<HookedFLAlertLayer*>(FLAlertLayer::create("Globed notice", notice, "Ok"));
            alert->setID("notice-popup"_spr);
            alert->blockClosingFor(BLOCK_CLOSING_FOR);
            alert->show();
        } else {
            log::warn("cant show flalert, ignoring notice: {}", notice);
        }
    }
}

bool ErrorCheckNode::canShowFLAlert() {
    auto* scene = CCScene::get();

    if (scene->getChildrenCount() == 0 || !scene->getChildren()) return true;

    size_t flalerts = 0;

    for (auto* child : CCArrayExt<CCNode*>(scene->getChildren())) {
        if (typeinfo_cast<FLAlertLayer*>(child)) {
            if (child->getID() == "error-popup"_spr || child->getID() == "notice-popup"_spr) {
                flalerts++;
            }
        }
    }

    // if there are already 2+ alerts on the screen, don't show any more
    return flalerts < 2;
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