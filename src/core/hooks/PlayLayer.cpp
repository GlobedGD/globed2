#include "GJBaseGameLayer.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <globed/util/singleton.hpp>
#include <core/PreloadManager.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedPlayLayer : geode::Modify<HookedPlayLayer, PlayLayer> {
    GlobedGJBGL* asBase() {
        return GlobedGJBGL::get(this);
    }

    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayLayer::destroyPlayer", 0x500000);
    }

    $override
    bool init(GJGameLevel* level, bool a, bool b) {
        auto gjbgl = this->asBase();
        gjbgl->setupPreInit(level, false);

        if (!PlayLayer::init(level, a, b)) return false;

        gjbgl->setupPostInit();

        return true;
    }

    $override
    void onQuit() {
        this->asBase()->onQuit();
        PlayLayer::onQuit();
    }

    $override
    void setupHasCompleted() {
        // This function calls `GameManager::loadDeathEffect` (inlined on windows),
        // which unloads the current death effect (this interferes with our preloading mechanism).
        // Override it and temporarily set death effect to 0, to prevent it from being unloaded.

        auto& pm = PreloadManager::get();
        if (!pm.deathEffectsLoaded()) {
            // if they were not preloaded, skip custom behavior and let the game handle it normally
            PlayLayer::setupHasCompleted();
            return;
        }

        auto gm = globed::cachedSingleton<GameManager>();
        int effect = gm->m_playerDeathEffect;

        gm->m_loadedDeathEffect = 0;
        gm->m_playerDeathEffect = 0;

        PlayLayer::setupHasCompleted();

        // TODO: the progress bar indicators

        gm->m_playerDeathEffect = effect;
        gm->m_loadedDeathEffect = effect;
    }

    $override
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        PlayLayer::destroyPlayer(player, obj);

        if (obj != m_anticheatSpike) {
            GlobedGJBGL::get(this)->handleLocalPlayerDeath(player);
        }
    }
};

}
