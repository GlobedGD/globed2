#include "loading_layer.hpp"

#include "game_manager.hpp"
#include <util/format.hpp>
#include <util/time.hpp>
#include <util/cocos.hpp>
#include <util/debug.hpp>
#include <util/lowlevel.hpp>

using namespace geode::prelude;

static void loadingFinishedReimpl(bool fromRefresh) {
    // reimplementation of the function
    auto* scene = MenuLayer::scene(fromRefresh);
    auto* dir = CCDirector::get();
    dir->replaceScene(scene);
}

#ifndef GLOBED_LOADING_FINISHED_MIDHOOK
void HookedLoadingLayer::loadingFinished() {
    this->loadingFinishedHook();
}
#endif

void HookedLoadingLayer::loadingFinishedHook() {
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
        this->scheduleOnce(schedule_selector(HookedLoadingLayer::preloadingStage2), 0.0f);
        return;
    } else if (m_fields->preloadingStage == 1000) {
        log::info("Asset preloading finished in {}.", util::format::formatDuration(util::time::systemNow() - m_fields->loadingStartedTime));
        util::cocos::cleanupThreadPool();
        loadingFinishedReimpl(m_fromRefresh);
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
    this->loadingFinishedHook();
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

static void loadingFinishedCaller() {
    auto* scene = CCScene::get();
    if (!scene) return loadingFinishedReimpl(false);
    if (!scene->getChildrenCount()) return loadingFinishedReimpl(false);

    for (auto* child : CCArrayExt<CCNode*>(scene->getChildren())) {
        if (auto* load = typeinfo_cast<LoadingLayer*>(child)) {
            static_cast<HookedLoadingLayer*>(load)->loadingFinishedHook();
            return;
        }
    }
}

// LoadingLayer::loadingFinished is inlined on Mac and Windows.
// This is fucked up.
// Please help.
#ifdef GLOBED_LOADING_FINISHED_MIDHOOK

# ifdef GEODE_IS_ARM_MAC
#  pragma message("loadingFinished midhook not implemented on arm mac")
# elif GEODE_COMP_GD_VERSION != 22060
#  pragma message("loadingFinished midhook not implemented for this GD version")
# else

// LoadingLayer::loadAssets, invokation of MenuLayer::scene
// win64 disasm (1):
/*
    xor ecx, ecx                      <- offset for start1
    call MenuLayer::scene
    mov rbx, rax
    call CCDirector::sharedDirector
    mov rcx, rax
    mov rdx, rbx
    call CCDirector::replaceScene
    jmp <out>                        <- offset for end1

    mov cl, 0x1                      <- offset for start2
    call MenuLayer::scene
    mov rbx, rax
    call CCDirector::sharedDirector
    mov rcx, rax
    mov rdx, rbx
    call CCDirector::replaceScene
    jmp <out>                        <- offset for end2

    int3
*/
#ifdef GEODE_IS_WINDOWS
const auto INLINED_PATCH_SPOTS = std::to_array<std::pair<ptrdiff_t, ptrdiff_t>>({
    {0x30ee46, 0x30ee62},
    {0x30ee67, 0x30ee83},
});
#else
# pragma message("todo add inlined mac addresses")
const auto INLINED_PATCH_SPOTS = std::to_array<std::pair<ptrdiff_t, ptrdiff_t>>({
});
#endif

$execute {
    std::vector<uint8_t> patchBytes;

    // movabs rax, <loadingFinishedCaller>
    patchBytes.push_back(0x48);
    patchBytes.push_back(0xb8);
    for (auto byte : geode::toByteArray(&loadingFinishedCaller)) {
        patchBytes.push_back(byte);
    }

    // call rax
    patchBytes.push_back(0xff);
    patchBytes.push_back(0xd0);

    for (const auto [start, end] : INLINED_PATCH_SPOTS) {
        GLOBED_REQUIRE(start + patchBytes.size() <= end, "unable to patch, not enough space")

        std::vector patchBytesCopy(patchBytes);

        // nop out the rest
        for (size_t i = start + patchBytes.size(); i < end; i++) {
            patchBytesCopy.push_back(0x90);
        }

        auto patch = util::lowlevel::patch(start, patchBytesCopy);

        if (patch && !patch->isEnabled()) {
            auto res = patch->enable();
            if (!res) {
                log::warn("Failed to patch loadingFinished midhook at {:X}: {}", start, res.unwrapErr());
            }
        }
    }
}

# endif
#endif // GLOBED_LOADING_FINISHED_MIDHOOK
