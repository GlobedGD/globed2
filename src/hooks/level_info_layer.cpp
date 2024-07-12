#include "level_info_layer.hpp"

#include <hooks/game_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/daily_manager.hpp>
#include <managers/admin.hpp>
#include <net/manager.hpp>
#include <ui/menu/level_list/level_list_layer.hpp>
#include <ui/menu/featured/edit_featured_level_popup.hpp>
#include <ui/menu/featured/featured_list_layer.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

static int& storedRateTier() {
    return static_cast<HookedGameManager*>(GameManager::get())->m_fields->lastLevelRateTier;
}

bool HookedLevelInfoLayer::init(GJGameLevel* level, bool challenge) {
    if (!LevelInfoLayer::init(level, challenge)) return false;

    auto& am = AdminManager::get();
    if (am.authorized() && am.getRole().canModerate()) {
        this->addLevelSendButton();
    }

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

void HookedLevelInfoLayer::addLevelSendButton() {
    // the positioning is fundamentally different with and without node ids lol
    bool nodeids = Loader::get()->isModLoaded("geode.node-ids");

    auto makeButton = [this]{
        bool plat = this->m_level->isPlatformer();
        return Build<CCSprite>::createSpriteName("icon-send-btn.png"_spr)
            .intoMenuItem([this, plat] {
                if (plat) {
                    EditFeaturedLevelPopup::create(this->m_level)->show();
                } else {
                    FLAlertLayer::create("Error", "Only <cj>Platformer levels</c> are eligible to be <cg>Globed Featured!</c>", "Ok")->show();
                }
            })
            .with([&](auto* btn) {
                if (!plat) {
                    btn->setColor({100, 100, 100});
                }
            })
            .id("send-btn"_spr)
            .collect();
    };

    if (nodeids) {
        auto* leftMenu = typeinfo_cast<CCMenu*>(this->getChildByIDRecursive("left-side-menu"));
        if (!leftMenu) {
            return;
        }

        Build(makeButton())
            .parent(leftMenu);

        leftMenu->updateLayout();
    } else {
        auto* menu = getChildOfType<CCMenu>(this, 1);

        if (!menu || menu->getChildrenCount() == 0) {
            log::warn("Failed to find left-side-menu");
            return;
        }

        CCMenuItemSpriteExtra* copybtn = nullptr;

        // find copy button
        size_t idx = menu->getChildrenCount();

        do {
            idx--;
            if (auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildren()->objectAtIndex(idx))) {
                if (isSpriteFrameName(static_cast<CCSprite*>(btn->getChildren()->objectAtIndex(0)), "GJ_duplicateBtn_001.png")) {
                    copybtn = btn;
                    break;
                }
            }
        } while (idx > 0);

        if (!copybtn) {
            log::warn("failed to find copy button");
            return;
        }

        Build(makeButton())
            .parent(menu)
            .pos(copybtn->getPosition() - CCPoint{0.f, 50.f});
    }
}
