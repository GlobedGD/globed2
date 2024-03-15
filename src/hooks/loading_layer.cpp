#include "loading_layer.hpp"

#include "game_manager.hpp"
#include <util/format.hpp>
#include <util/time.hpp>
#include <util/cocos.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;

void HookedLoadingLayer::loadingFinished() {
    if (m_fields->preloadingStage == 0) {
        m_fields->loadingStartedTime = util::time::systemNow();

        m_fields->preloadingStage++;

        auto* gm = GameManager::get();
        auto* hgm = static_cast<HookedGameManager*>(gm);

        if (!util::cocos::shouldTryToPreload(true)) {
            log::info("Asset preloading disabled/deferred, not loading anything.");
            this->finishLoading();
            return;
        }

        log::info("preloading assets");
        this->setLabelText("Globed: preloading death effects");

        // start the stage 1 - load death effects
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
        return;
    } else if (m_fields->preloadingStage == 1000) {
        log::info("Asset preloading finished in {}.", util::format::formatDuration(util::time::systemNow() - m_fields->loadingStartedTime));
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
        case 1: this->setLabelText("Globed: preloading death effects"); break;
        case 2: this->setLabelText("Globed: preloading cube icons"); break;
        case 3: this->setLabelText("Globed: preloading ship icons"); break;
        case 4: this->setLabelText("Globed: preloading ball icons"); break;
        case 5: this->setLabelText("Globed: preloading UFO icons"); break;
        case 6: this->setLabelText("Globed: preloading wave icons"); break;
        case 7: this->setLabelText("Globed: preloading other icons"); break;
    }
}

void HookedLoadingLayer::finishLoading() {
    m_fields->preloadingStage = 1000;
    LoadingLayer::loadingFinished();
}

void HookedLoadingLayer::preloadingStage1(float) {
    m_fields->preloadingStage++;
    this->setLabelTextForStage();
    this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.05f);
}

void HookedLoadingLayer::preloadingStage2(float) {
    using util::cocos::AssetPreloadStage;
    using util::cocos::preloadAssets;

    auto& settings = GlobedSettings::get();

    switch (m_fields->preloadingStage) {
    // death effects
    case 1: {
        // only preload them if they are enabled, they take like half the loading time
        if (settings.players.deathEffects && !settings.players.defaultDeathEffect) {
            preloadAssets(AssetPreloadStage::DeathEffect);
            static_cast<HookedGameManager*>(GameManager::get())->setDeathEffectsPreloaded(true);
        }
    } break;
    case 2: preloadAssets(AssetPreloadStage::Cube); break;
    case 3: preloadAssets(AssetPreloadStage::Ship); break;
    case 4: preloadAssets(AssetPreloadStage::Ball); break;
    case 5: preloadAssets(AssetPreloadStage::Ufo); break;
    case 6: preloadAssets(AssetPreloadStage::Wave); break;
    case 7: preloadAssets(AssetPreloadStage::Other); break;
    case 8: {
        static_cast<HookedGameManager*>(GameManager::get())->setAssetsPreloaded(true);
        this->finishLoading();
        return;
    }
    };

    m_fields->preloadingStage++;
    this->setLabelTextForStage();

    if (m_fields->preloadingStage % 2 == 1) {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.0f);
    } else {
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage3), 0.0f);
    }
}

void HookedLoadingLayer::preloadingStage3(float x) {
    // this is same as 2 lmao
    this->preloadingStage2(x);
}