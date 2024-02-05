#include "pause_layer.hpp"

#include <ui/game/userlist/userlist.hpp>

using namespace geode::prelude;

void GlobedPauseLayer::customSetup() {
    PauseLayer::customSetup();

    // TODO temp button placement!

    Build<CCSprite>::createSpriteName("GJ_profileButton_001.png")
        .intoMenuItem([](auto) {
            GlobedUserListPopup::create()->show();
        })
        .id("btn-open-playerlist"_spr)
        .parent(this);
}