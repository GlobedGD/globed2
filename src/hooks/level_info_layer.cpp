#include "level_info_layer.hpp"
#include <net/manager.hpp>
#include <managers/daily_manager.hpp>

using namespace geode::prelude;

bool HookedLevelInfoLayer::init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;
    //log::info("detected: {}", DailyManager::get().rateTierOpen);

    int rating = DailyManager::get().rateTierOpen;

    if (rating != -1) {
        GJDifficultySprite* diff = typeinfo_cast<GJDifficultySprite*>(this->getChildByIDRecursive("difficulty-sprite"));
        if (!diff) {
            for (auto* child : CCArrayExt<CCNode*>(this->getChildren())) {
                if (auto p = getChildOfType<GJDifficultySprite>(child, 0)) {
                    diff = p;
                    break;
                }
            }
        }

        if (diff) {
            DailyManager::get().attachRatingSprite(rating, diff);
        }
    }

    return true;
}

void HookedLevelInfoLayer::onPlay(CCObject* s) {
    if (m_fields->allowOpeningAnyway) {
        m_fields->allowOpeningAnyway = false;
        LevelInfoLayer::onPlay(s);
        return;
    }

    // if we are already in a playlayer, don't allow sillyness
    if (NetworkManager::get().established() && GJBaseGameLayer::get() != nullptr) {
        geode::createQuickPopup("Warning", "You are already inside of a level, opening another level may cause Globed to crash. <cy>Do you still want to proceed?</c>", "Cancel", "Play", [this, s](auto, bool play) {
            if (play) {
                this->forcePlay(s);
            }
        });
        return;
    }

    LevelInfoLayer::onPlay(s);
}

void HookedLevelInfoLayer::forcePlay(CCObject* s) {
    m_fields->allowOpeningAnyway = true;
    LevelInfoLayer::onPlay(s);
}

void HookedLevelInfoLayer::tryCloneLevel(CCObject* s) {
    if (NetworkManager::get().established() && GJBaseGameLayer::get() != nullptr) {
        FLAlertLayer::create("Globed Error", "Cannot perform this action while in a level", "Ok")->show();
        return;
    }

    LevelInfoLayer::tryCloneLevel(s);
}