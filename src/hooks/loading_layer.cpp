#include "loading_layer.hpp"

#include "game_manager.hpp"
#include <util/format.hpp>
#include <util/time.hpp>

using namespace geode::prelude;

// [from; to]
#define LOAD_ICON_RANGE(type, from, to) { \
    for (int i = from; i <= to; i++) { \
        gm->loadIcon(i, (int)IconType::type, -1); \
    } }

// [from; to]
#define LOAD_DEATH_EFFECT_RANGE(from, to) { \
    for (int i = from; i <= to; i++) { \
        util::ui::tryLoadDeathEffect(i); \
    } \
}

void HookedLoadingLayer::loadingFinished() {
    if (m_fields->preloadingStage == 0) {
        m_fields->loadingStartedTime = util::time::systemNow();
        log::debug("preloading assets");

        m_fields->preloadingStage++;
        this->setLabelText("Globed: preloading death effects");

        auto* gm = GameManager::get();
        auto* hgm = static_cast<HookedGameManager*>(gm);
        hgm->m_fields->iconCache.clear();

        if (Loader::get()->getLaunchFlag("globed-skip-preload") || Loader::get()->getLaunchFlag("globed-skip-death-effects") || Mod::get()->getSettingValue<bool>("force-skip-preload")) {
            log::warn("Asset preloading is disabled. Not loading anything.");
            this->finishLoading();
            return;
        }

        auto& settings = GlobedSettings::get();
        if (!settings.globed.preloadAssets) {
            this->finishLoading();
            return;
        }

        // start the stage 1 - load death effects
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
        return;
    } else if (m_fields->preloadingStage == 1000) {
        log::debug("Asset preloading finished in {}.", util::format::formatDuration(util::time::systemNow() - m_fields->loadingStartedTime));
        LoadingLayer::loadingFinished();
    }
}

void HookedLoadingLayer::setLabelText(const std::string_view text) {
    auto label = static_cast<CCLabelBMFont*>(this->getChildByID("geode-small-label"));

    if (label) {
        label->setString(std::string(text).c_str());
    }
}

void HookedLoadingLayer::setLabelTextForStage() {
    int stage = m_fields->preloadingStage;
    switch (stage) {
    case 1: [[fallthrough]];
    case 2: [[fallthrough]];
    case 3: [[fallthrough]];
    case 4: this->setLabelText("Globed: preloading death effects"); break;

    case 5: [[fallthrough]];
    case 6: [[fallthrough]];
    case 7: [[fallthrough]];
    case 8: [[fallthrough]];
    case 9: this->setLabelText("Globed: preloading cube icons"); break;

    case 10: [[fallthrough]];
    case 11: this->setLabelText("Globed: preloading ship icons"); break;

    case 12: [[fallthrough]];
    case 13: this->setLabelText("Globed: preloading ball icons"); break;

    case 14: [[fallthrough]];
    case 15: this->setLabelText("Globed: preloading UFO icons"); break;

    case 16: [[fallthrough]];
    case 17: this->setLabelText("Globed: preloading wave icons"); break;

    case 18: this->setLabelText("Globed: preloading robot icons"); break;
    case 19: this->setLabelText("Globed: preloading spider icons"); break;
    case 20: this->setLabelText("Globed: preloading swing icons"); break;
    case 21: this->setLabelText("Globed: preloading jetpack icons"); break;
    }
}

void HookedLoadingLayer::finishLoading() {
    m_fields->preloadingStage = 1000;
    LoadingLayer::loadingFinished();
}

void HookedLoadingLayer::preloadingStage1(float) {
    for (int i = 1; i < 21; i++) {
        util::ui::tryLoadDeathEffect(i);
    }

    m_fields->preloadingStage++;
    this->setLabelTextForStage();
    this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
}

void HookedLoadingLayer::preloadingStage2(float) {
    auto* gm = GameManager::get();

    switch (m_fields->preloadingStage) {
    case 1: LOAD_DEATH_EFFECT_RANGE(1, 5); break;
    case 2: LOAD_DEATH_EFFECT_RANGE(6, 11); break;
    case 3: LOAD_DEATH_EFFECT_RANGE(12, 16); break;
    case 4: LOAD_DEATH_EFFECT_RANGE(17, 21); break;

    case 5: LOAD_ICON_RANGE(Cube, 0, 100); break;
    case 6: LOAD_ICON_RANGE(Cube, 101, 200); break;
    case 7: LOAD_ICON_RANGE(Cube, 201, 300); break;
    case 8: LOAD_ICON_RANGE(Cube, 301, 400); break;
    case 9: LOAD_ICON_RANGE(Cube, 401, 484); break;

    case 10: LOAD_ICON_RANGE(Ship, 0, 80); break;
    // There are actually 169 ship icons, but for some reason, loading the last icon causes
    // a very strange bug when you have the Default mini icons option enabled.
    // I have no idea how loading a ship icon can cause a ball icon to become a cube,
    // and honestly I don't care enough.
    // https://github.com/dankmeme01/globed2/issues/93
    case 11: LOAD_ICON_RANGE(Ship, 81, 168); break;

    case 12: LOAD_ICON_RANGE(Ball, 0, 60); break;
    case 13: LOAD_ICON_RANGE(Ball, 61, 118); break;

    case 14: LOAD_ICON_RANGE(Ufo, 0, 75); break;
    case 15: LOAD_ICON_RANGE(Ufo, 76, 149); break;

    case 16: LOAD_ICON_RANGE(Wave, 0, 50); break;
    case 17: LOAD_ICON_RANGE(Wave, 51, 96); break;

    case 18: LOAD_ICON_RANGE(Robot, 0, 68); break;
    case 19: LOAD_ICON_RANGE(Spider, 0, 69); break;
    case 20: LOAD_ICON_RANGE(Swing, 0, 43); break;
    case 21: LOAD_ICON_RANGE(Jetpack, 0, 5); break;
    case 22: {
        this->finishLoading();
        return;
    }
    };

    m_fields->preloadingStage++;
    this->setLabelTextForStage();

    if (m_fields->preloadingStage % 2 == 1) {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
    } else {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage3), 0.05f);
    }
}

void HookedLoadingLayer::preloadingStage3(float x) {
    // this is same as 2 lmao
    this->preloadingStage2(x);
}