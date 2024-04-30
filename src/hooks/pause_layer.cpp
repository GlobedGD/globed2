#include "pause_layer.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <ui/game/userlist/userlist.hpp>
#include <ui/game/chat/chatlist.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

void GlobedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto* gpl = GlobedGJBGL::get();

    if (!gpl || !gpl->m_fields->globedReady) return;

    auto winSize = CCDirector::get()->getWinSize();

    Build<CCSprite>::createSpriteName("GJ_profileButton_001.png")
        .scale(0.9f)
        .intoMenuItem([](auto) {
            GlobedUserListPopup::create()->show();
        })
        .pos(winSize.width - 50.f, 50.f)
        .id("btn-open-playerlist"_spr)
        .intoNewParent(CCMenu::create())
        .id("playerlist-menu"_spr)
        .pos(0.f, 0.f)
        .parent(this);

    Build<CCSprite>::createSpriteName("accountBtn_messages_001.png")
        .scale(0.9f)
        .intoMenuItem([](auto) {
            GlobedChatListPopup::create()->show();
        })
        .pos(winSize.width - 50.f, 100.f)
        .id("btn-open-chatlist"_spr)
        .intoNewParent(CCMenu::create())
        .id("chatlist-menu"_spr)
        .pos(0.f, 0.f)
        .parent(this);

    this->schedule(schedule_selector(GlobedPauseLayer::selUpdate), 0.f);
}

void GlobedPauseLayer::selUpdate(float dt) {
    auto* pl = GlobedGJBGL::get();

    if (pl) {
        pl->pausedUpdate(dt);
    }
}

void GlobedPauseLayer::goEdit() {
    auto* pl = GlobedGJBGL::get();
    if (pl) {
        // make sure we remove listeners and stuff
        pl->onQuitActions();
    }

    PauseLayer::goEdit();
}

bool GlobedPauseLayer::hasPopup() {
    if (!this->getParent()) return false;

    return getChildOfType<GlobedUserListPopup>(this->getParent(), 0) != nullptr;
}

#define REPLACE(method) \
    void GlobedPauseLayer::method(CCObject* s) {\
        if (!this->hasPopup()) { \
            PauseLayer::method(s); \
        } \
    }

REPLACE(onQuit);
REPLACE(onResume);
REPLACE(onRestart);
REPLACE(onRestartFull);
REPLACE(onEdit);
REPLACE(onNormalMode);
REPLACE(onPracticeMode);

/* bugfix */

void PauseLayerBugfix::customSetup() {
    PauseLayer::customSetup();

    // vmt hook
#ifdef GEODE_IS_WINDOWS
    static auto patch = [&]() -> Patch* {
        auto vtableidx = 87;
        auto p = util::lowlevel::vmtHook(&PauseLayerBugfix::onExitHook, this, vtableidx);
        if (p.isErr()) {
            log::warn("vmt hook failed: {}", p.unwrapErr());
            return nullptr;
        } else {
            return p.unwrap();
        }
    }();

    if (patch && !patch->isEnabled()) {
        (void) patch->enable();
    }
#endif
}

void PauseLayerBugfix::onExitHook() {
    // close any popups
    auto* arr = this->getParent()->getChildren();
    for (size_t idx = arr->count(); idx > 0; idx--) {
        auto* obj = arr->objectAtIndex(idx - 1);
        if (auto* alert = typeinfo_cast<FLAlertLayer*>(obj)) {
            alert->retain();
            alert->keyBackClicked();
            Loader::get()->queueInMainThread([alert] {
                alert->release();
            });
        }
    }

    PauseLayer::onExit();
}
