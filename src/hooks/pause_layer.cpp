#include "pause_layer.hpp"

#include <hooks/play_layer.hpp>
#include <ui/game/userlist/userlist.hpp>

using namespace geode::prelude;

void GlobedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    if (!static_cast<GlobedPlayLayer*>(PlayLayer::get())->m_fields->globedReady) return;

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
