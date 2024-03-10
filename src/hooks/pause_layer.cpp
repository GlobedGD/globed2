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

#define REPLACE(method) \
    void GlobedPauseLayer::method(CCObject* s) {\
        if (getChildOfType<GlobedUserListPopup>(this->getParent(), 0) == nullptr) { \
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
