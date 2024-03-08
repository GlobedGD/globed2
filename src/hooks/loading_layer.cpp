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
    using BatchedIconRange = HookedGameManager::BatchedIconRange;

    auto* gm = static_cast<HookedGameManager*>(GameManager::get());

    switch (m_fields->preloadingStage) {
    // death effects
    case 1: {
        std::vector<std::string> images;

        for (size_t i = 1; i < 20; i++) {
            auto key = fmt::format("PlayerExplosion_{:02}", i);
            images.push_back(key);
        }

        util::cocos::loadAssetsParallel(images);
    } break;

    case 2: gm->loadIconsBatched((int)IconType::Cube, 0, 484); break;

    // There are actually 169 ship icons, but for some reason, loading the last icon causes
    // a very strange bug when you have the Default mini icons option enabled.
    // I have no idea how loading a ship icon can cause a ball icon to become a cube,
    // and honestly I don't care enough.
    // https://github.com/dankmeme01/globed2/issues/93
    case 3: gm->loadIconsBatched((int)IconType::Ship, 1, 168); break;

    case 4: gm->loadIconsBatched((int)IconType::Ball, 0, 118); break;
    case 5: gm->loadIconsBatched((int)IconType::Ufo, 1, 149); break;
    case 6: gm->loadIconsBatched((int)IconType::Wave, 1, 96); break;
    case 7: {
        std::vector<BatchedIconRange> ranges = {
            BatchedIconRange{
                .iconType = (int)IconType::Robot,
                .startId = 1,
                .endId = 68
            },
            BatchedIconRange{
                .iconType = (int)IconType::Spider,
                .startId = 1,
                .endId = 69
            },
            BatchedIconRange{
                .iconType = (int)IconType::Swing,
                .startId = 1,
                .endId = 43
            },
            BatchedIconRange{
                .iconType = (int)IconType::Jetpack,
                .startId = 1,
                .endId = 5
            },
        };

        gm->loadIconsBatched(ranges);
    } break;
    case 8: {
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