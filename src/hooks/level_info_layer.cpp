#include "level_info_layer.hpp"

using namespace geode::prelude;

void HookedLevelInfoLayer::onPlay(CCObject* s) {
    // if we are already in a playlayer, don't allow sillyness

    if (PlayLayer::get() != nullptr) {
        FLAlertLayer::create("Globed Error", "Cannot open a level while already in another level", "Ok")->show();
        return;
    }

    LevelInfoLayer::onPlay(s);
}

void HookedLevelInfoLayer::tryCloneLevel(CCObject* s) {
    if (PlayLayer::get() != nullptr) {
        FLAlertLayer::create("Globed Error", "Cannot perform this action while in a level", "Ok")->show();
        return;
    }

    LevelInfoLayer::tryCloneLevel(s);
}