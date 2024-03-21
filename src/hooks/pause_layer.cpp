#include "pause_layer.hpp"

#include <hooks/play_layer.hpp>
#include <ui/game/userlist/userlist.hpp>

using namespace geode::prelude;

void GlobedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    auto* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get());

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

    this->schedule(schedule_selector(GlobedPauseLayer::selUpdate), 0.f);
}

void GlobedPauseLayer::selUpdate(float dt) {
    auto* pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    if (pl) {
        pl->pausedUpdate(dt);
    }
}

void GlobedPauseLayer::goEdit() {
    auto* pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
    if (pl) {
        // make sure we remove listeners and stuff
        pl->onQuitActions();
    }

    PauseLayer::goEdit();
}

bool GlobedPauseLayer::hasPopup(bool allPopups) {
    if (allPopups) {
        return getChildOfType<FLAlertLayer>(this->getParent(), 0) != nullptr;
    } else {
        return getChildOfType<GlobedUserListPopup>(this->getParent(), 0) != nullptr;
    }
}

#define REPLACE(method) \
    void GlobedPauseLayer::method(CCObject* s) {\
        if (!this->hasPopup(false)) { \
            PauseLayer::method(s); \
        } \
    }

// yet another robtop bug zz
#define REPLACE_CHECKED(method) \
    void GlobedPauseLayer::method(CCObject* s) {\
        if (!this->hasPopup(true)) { \
            PauseLayer::method(s); \
        } \
    }

REPLACE(onQuit);
REPLACE_CHECKED(onResume);
REPLACE_CHECKED(onRestart);
REPLACE(onRestartFull);
REPLACE_CHECKED(onEdit);
REPLACE_CHECKED(onNormalMode);
REPLACE_CHECKED(onPracticeMode);
