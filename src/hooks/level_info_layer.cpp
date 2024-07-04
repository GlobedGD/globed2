#include "level_info_layer.hpp"

#include <hooks/game_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/featured/featured_list_layer.hpp>
#include <managers/daily_manager.hpp>
#include <net/manager.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

static int& storedRateTier() {
    return static_cast<HookedGameManager*>(GameManager::get())->m_fields->lastLevelRateTier;
}

bool HookedLevelInfoLayer::init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;

    // i hate myself
    if (level->m_levelIndex == 0) {
        for (auto child : CCArrayExt<CCNode*>(CCScene::get()->getChildren())) {
            if (typeinfo_cast<GlobedLevelListLayer*>(child) || typeinfo_cast<GlobedFeaturedListLayer*>(child)) {
                util::gd::reorderDownloadedLevel(level);
                break;
            }
        }
    }

    int rating = DailyManager::get().rateTierOpen;

    bool isQuittingPlayLayer = false;
    if (auto pl = GlobedGJBGL::get()) {
        isQuittingPlayLayer = pl->m_fields->quitting;
    }

    if (rating == -1 && storedRateTier() != -1 && (!PlayLayer::get() || isQuittingPlayLayer)) {
        rating = storedRateTier();
        storedRateTier() = -1;
    }

    if (rating != -1) {
        if (auto* diff = DailyManager::get().findDifficultySprite(this)) {
            DailyManager::get().attachRatingSprite(rating, diff);
        }

        this->m_fields->rateTier = rating;
    }

    return true;
}

void HookedLevelInfoLayer::onPlay(CCObject* s) {
    if (m_fields->allowOpeningAnyway) {
        m_fields->allowOpeningAnyway = false;
        storedRateTier() = m_fields->rateTier;
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

    storedRateTier() = m_fields->rateTier;
    LevelInfoLayer::onPlay(s);
}

void HookedLevelInfoLayer::forcePlay(CCObject* s) {
    m_fields->allowOpeningAnyway = true;

    storedRateTier() = m_fields->rateTier;
    LevelInfoLayer::onPlay(s);
}

void HookedLevelInfoLayer::tryCloneLevel(CCObject* s) {
    if (NetworkManager::get().established() && GJBaseGameLayer::get() != nullptr) {
        FLAlertLayer::create("Globed Error", "Cannot perform this action while in a level", "Ok")->show();
        return;
    }

    LevelInfoLayer::tryCloneLevel(s);
}