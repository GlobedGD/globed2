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

void HookedLoadingLayer::loadingFinished() {
    if (m_fields->preloadingStage == 0) {
        m_fields->loadingStartedTime = util::time::systemNow();
        log::debug("preloading assets");

        m_fields->preloadingStage++;
        this->setLabelText("Globed: preloading death effects");

        auto* gm = GameManager::get();
        auto* hgm = static_cast<HookedGameManager*>(gm);
        hgm->m_fields->iconCache.clear();

        if (Loader::get()->getLaunchFlag("globed-skip-preload") || Loader::get()->getLaunchFlag("globed-skip-death-effects")) {
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
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage1), 0.05f);
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
    case 2: this->setLabelText("Globed: preloading cube icons"); break;
    case 3: this->setLabelText("Globed: preloading ship icons"); break;
    case 4: this->setLabelText("Globed: preloading ball icons"); break;
    case 5: this->setLabelText("Globed: preloading UFO icons"); break;
    case 6: this->setLabelText("Globed: preloading wave icons"); break;
    case 7: this->setLabelText("Globed: preloading robot icons"); break;
    case 8: this->setLabelText("Globed: preloading spider icons"); break;
    case 9: this->setLabelText("Globed: preloading swing icons"); break;
    case 10: this->setLabelText("Globed: preloading jetpack icons"); break;
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
    case 2: LOAD_ICON_RANGE(Cube, 0, 484); break;
    case 3: LOAD_ICON_RANGE(Ship, 0, 169); break;
    case 4: LOAD_ICON_RANGE(Ball, 0, 118); break;
    case 5: LOAD_ICON_RANGE(Ufo, 0, 149); break;
    case 6: LOAD_ICON_RANGE(Wave, 0, 96); break;
    case 7: LOAD_ICON_RANGE(Robot, 0, 68); break;
    case 8: LOAD_ICON_RANGE(Spider, 0, 69); break;
    case 9: LOAD_ICON_RANGE(Swing, 0, 43); break;
    case 10: LOAD_ICON_RANGE(Jetpack, 0, 5); break;
    case 11: {
        this->finishLoading();
        return;
    }
    };

    m_fields->preloadingStage++;
    this->setLabelTextForStage();

    if (m_fields->preloadingStage % 2 == 1) {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage3), 0.05f);
    } else {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
    }
}

void HookedLoadingLayer::preloadingStage3(float x) {
    // this is same as 2 lmao
    this->preloadingStage2(x);
}