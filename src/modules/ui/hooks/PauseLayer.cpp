#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <modules/ui/UIModule.hpp>
#include <modules/ui/popups/UserListPopup.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

static bool g_force = false;

struct GLOBED_MODIFY_ATTR UIHookedPauseLayer : Modify<UIHookedPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PauseLayer::customSetup", 10);
        (void) self.setHookPriority("PauseLayer::onQuit", -10000);
        (void) self.setHookPriority("PauseLayer::goEdit", -999999999);
        (void) self.setHookPriority("PauseLayer::onRestart", -99999);
        (void) self.setHookPriority("PauseLayer::onRestartFull", -99999);

        GLOBED_CLAIM_HOOKS(UIModule::get(), self,
            "PauseLayer::customSetup",
            "PauseLayer::onQuit",
            "PauseLayer::onEdit",
            "PauseLayer::goEdit",
            // "PauseLayer::onResume",
            // "PauseLayer::onNormalMode",
            // "PauseLayer::onPracticeMode",
            "PauseLayer::onRestart",
            "PauseLayer::onRestartFull",
        );
    }

    $override
    void customSetup() {
        PauseLayer::customSetup();

        auto gpl = GlobedGJBGL::get();
        if (!gpl || !gpl->active()) return;

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = Build<CCMenu>::create()
            .id("playerlist-menu"_spr)
            .parent(this)
            .pos(winSize.width - 28.f, 24.f)
            .anchorPoint(1.f, 0.f)
            .contentSize(48.f, winSize.height - 48.f)
            .layout(ColumnLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start))
            .collect();

        Build<CCSprite>::create("icon-players.png"_spr)
            .scale(0.9f)
            .intoMenuItem(+[] {
                UserListPopup::create()->show();
            })
            .id("btn-open-playerlist"_spr)
            .parent(menu);

        menu->updateLayout();

        this->schedule(schedule_selector(UIHookedPauseLayer::selUpdate), 0.f);
    }

    void selUpdate(float dt) {
        if (auto pl = GlobedGJBGL::get()) {
            pl->pausedUpdate(dt);
        }
    }

    $override
    void onQuit(CCObject* sender) {
        if (this->hasPopup()) return;

        if (g_force) {
            PauseLayer::onQuit(sender);
            g_force = false;
            return;
        }

        auto& rm = RoomManager::get();
        if (rm.isInFollowerRoom() && !rm.isOwner()) {
            globed::quickPopup(
                "Note",
                "You are in a <cy>follower room</c>, you <cr>cannot</c> leave the level unless the room owner leaves as well. "
                "Proceeding will cause you to <cj>leave the room</c>, do you want to continue?",
                "Cancel", "Leave",
                [this](auto, bool yes) {
                    if (yes) {
                        g_force = true;
                        NetworkManagerImpl::get().sendLeaveRoom();
                        PauseLayer::onQuit(nullptr);
                    }
                }
            );
        } else {
            PauseLayer::onQuit(sender);
        }
    }

    $override
    void goEdit() {
        if (auto pl = GlobedGJBGL::get()) {
            pl->onQuit();
        }

        PauseLayer::goEdit();
    }

    bool hasPopup() {
        auto parent = this->getParent();
        if (!parent) return false;

        return parent->getChildByType<UserListPopup>(0);
    }

#define REPLACE(method) \
    void method(CCObject* s) {\
        if (!this->hasPopup()) { \
            PauseLayer::method(s); \
        } \
    }

#define REPLACE_UNPAUSE(method) \
    void method(CCObject* s) {\
        if (!this->hasPopup()) { \
            if (auto* gpl = GlobedGJBGL::get()) { \
                bool shouldResume = true; \
                for (auto& mod : gpl->m_fields->modules) { \
                    shouldResume = shouldResume && mod->onUnpause(); \
                } \
                \
                if (!shouldResume) { \
                    return; \
                } \
            } \
            PauseLayer::method(s); \
        } \
    }

    // TODO:
    // REPLACE_UNPAUSE(onResume);
    // REPLACE_UNPAUSE(onNormalMode);
    // REPLACE_UNPAUSE(onPracticeMode);

    $override
    REPLACE(onEdit);

    $override
    void onRestart(CCObject* s) {
        if (this->hasPopup()) return;

        auto& fields = *GlobedGJBGL::get()->m_fields.self();

        fields.m_manualReset = true;
        PauseLayer::onRestart(s);
        fields.m_manualReset = false;
    }

    $override
    void onRestartFull(CCObject* s) {
        if (this->hasPopup()) return;

        auto& fields = *GlobedGJBGL::get()->m_fields.self();

        fields.m_manualReset = true;
        PauseLayer::onRestartFull(s);
        fields.m_manualReset = false;
    }
};

}
