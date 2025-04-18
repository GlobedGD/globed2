#include "error_check_node.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <hooks/flalertlayer.hpp>
#include <managers/error_queues.hpp>
#include <util/debug.hpp>
#include <data/packets/client/general.hpp>
#include <ui/general/ask_input_popup.hpp>

#include <asp/time/Duration.hpp>

using namespace geode::prelude;
using namespace asp::time;

constexpr auto BLOCK_CLOSING_FOR = Duration::fromMillis(375);

void askForNoticeReply(uint32_t replyId) {
    AskInputPopup::create("Enter Reply Text", [replyId](std::string_view text) {
        NetworkManager::get().send(NoticeReplyPacket::create(replyId, std::string(text)));
    }, 280, "Message", util::misc::STRING_PRINTABLE_INPUT, 0.4f)->show();
}

bool ErrorCheckNode::init() {
    if (!CCNode::init()) return false;

    this->schedule(schedule_selector(ErrorCheckNode::updateErrors), 0.1f);

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

    auto playlayer = GlobedGJBGL::get();
    if (playlayer && !playlayer->isPaused()) {
        return;
    }

    try {
        auto errors = ErrorQueues::get().getErrors();
        auto notices = ErrorQueues::get().getNotices();

        for (auto& error : errors) {
            if (canShowFLAlert()) {
                log::debug("showing error: {}", error);
                auto alert = static_cast<HookedFLAlertLayer*>(
                    FLAlertLayer::create(nullptr, "Globed error", error, "Ok", nullptr, 360.f)
                );

                alert->setID("error-popup"_spr);
                alert->blockClosingFor(BLOCK_CLOSING_FOR);
                alert->show();
            } else {
                log::warn("cant show flalert, ignoring error: {}", error);
            }
        }

        for (auto& notice : notices) {
            if (canShowFLAlert()) {
                log::debug("showing notice: {} (reply id: {})", notice.message, notice.replyId);

                FLAlertLayer* alert;

                // if we cannot reply, just show a regular flalertlayer
                if (notice.replyId == 0) {
                    alert = FLAlertLayer::create(
                        nullptr,
                        "Globed notice",
                        notice.message,
                        "Ok",
                        nullptr,
                        380.f
                    );
                } else if (!notice.isReplyFrom.empty()) {
                    // if this is a reply itself from a user
                    alert = FLAlertLayer::create(
                        nullptr,
                        fmt::format("Reply from {}", notice.isReplyFrom).c_str(),
                        notice.message,
                        "Ok",
                        nullptr,
                        380.f
                    );
                } else {
                    // if we can reply, instead we show an alert with 2 buttons
                    alert = geode::createQuickPopup(
                        "Globed notice",
                        notice.message,
                        "Ok",
                        "Reply",
                        380.f,
                        [replyId = notice.replyId](auto, bool reply) {
                            if (!reply) return;

                            askForNoticeReply(replyId);
                    }, false);
                }

                alert->setID("notice-popup"_spr);
                static_cast<HookedFLAlertLayer*>(alert)->blockClosingFor(BLOCK_CLOSING_FOR);
                alert->show();
            } else {
                log::warn("cant show flalert, ignoring notice: {}", notice.message);
            }
        }
    } catch (const std::exception& e) {
        log::warn("failed to pop the errors/notices: {}", e.what());
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