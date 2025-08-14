#include "GJBaseGameLayer.hpp"
#include <globed/util/singleton.hpp>
#include <globed/util/gd.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/PreloadManager.hpp>

#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedPlayLayer : geode::Modify<HookedPlayLayer, PlayLayer> {
    struct Fields {
        bool m_setupWasCompleted = false;
    };

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

        m_fields->m_setupWasCompleted = true;
    }

    $override
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        auto& rm = RoomManager::get();
        bool overrideReset = !rm.isInGlobal() && this->asBase()->active();
        bool oldReset = overrideReset ? gameVariable(GameVariable::FastRespawn) : false;

        if (overrideReset) {
            setGameVariable(GameVariable::FastRespawn, rm.getSettings().fasterReset);
        }

        PlayLayer::destroyPlayer(player, obj);

        if (overrideReset) {
            setGameVariable(GameVariable::FastRespawn, oldReset);
        }

        if (obj != m_anticheatSpike) {
            GlobedGJBGL::get(this)->handleLocalPlayerDeath(player);
        }
    }

    $override
    void resetLevel() {
        PlayLayer::resetLevel();

        if (!m_fields->m_setupWasCompleted) return;

        auto gjbgl = GlobedGJBGL::get(this);
        if (!m_level->isPlatformer()) {
            // turn off safe mode in non-platformer levels (as this counts as a full reset)
            gjbgl->resetSafeMode();
        }
    }

    $override
    void fullReset() {
        PlayLayer::fullReset();

        auto gjbgl = GlobedGJBGL::get(this);
        gjbgl->resetSafeMode();
    }

    $override
    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        // doesn't actually change any progress but this stops the NEW BEST popup from showing up while cheating/jumping to a player
        if (!GlobedGJBGL::get(this)->isSafeMode()) PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
    }
};

}
